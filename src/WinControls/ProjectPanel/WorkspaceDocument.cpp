#include "WorkspaceDocument.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QObject>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

static WorkspaceNode readWorkspaceNode(QXmlStreamReader& reader,
                                       const QString& workspaceDirectory)
{
    WorkspaceNode node;
    const QString element = reader.name().toString();
    node.type = element == QStringLiteral("Project")
        ? WorkspaceNode::Project
        : (element == QStringLiteral("File")
            ? WorkspaceNode::File : WorkspaceNode::Folder);
    node.name = reader.attributes().value(QStringLiteral("name")).toString();
    if (node.type == WorkspaceNode::File) {
        const QString configuredPath = QDir::fromNativeSeparators(node.name);
        node.filePath = QDir::isAbsolutePath(configuredPath)
            ? QDir::cleanPath(configuredPath)
            : QDir(workspaceDirectory).absoluteFilePath(configuredPath);
        node.name = QFileInfo(configuredPath).fileName();
    }
    while (reader.readNextStartElement()) {
        if (reader.name() == QStringLiteral("Folder") ||
            reader.name() == QStringLiteral("File"))
            node.children.append(readWorkspaceNode(reader, workspaceDirectory));
        else
            reader.skipCurrentElement();
    }
    return node;
}

bool WorkspaceDocument::load(const QString& filePath, QString* errorMessage)
{
    QFile file(filePath);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        if (errorMessage)
            *errorMessage = file.errorString();
        return false;
    }
    QXmlStreamReader reader(&file);
    QVector<WorkspaceNode> loaded;
    if (!reader.readNextStartElement() ||
        reader.name() != QStringLiteral("NotepadPlus")) {
        if (errorMessage)
            *errorMessage = QObject::tr("Workspace root must be NotepadPlus.");
        return false;
    }
    while (reader.readNextStartElement()) {
        if (reader.name() == QStringLiteral("Project"))
            loaded.append(readWorkspaceNode(
                reader, QFileInfo(filePath).absolutePath()));
        else
            reader.skipCurrentElement();
    }
    if (reader.hasError() || loaded.isEmpty()) {
        if (errorMessage)
            *errorMessage = reader.hasError()
                ? reader.errorString()
                : QObject::tr("Workspace contains no Project element.");
        return false;
    }
    _projects = loaded;
    _filePath = QFileInfo(filePath).absoluteFilePath();
    return true;
}

static void writeWorkspaceNode(QXmlStreamWriter& writer,
                               const WorkspaceNode& node,
                               const QString& workspaceDirectory)
{
    const QString element = node.type == WorkspaceNode::Project
        ? QStringLiteral("Project")
        : (node.type == WorkspaceNode::File
            ? QStringLiteral("File") : QStringLiteral("Folder"));
    writer.writeStartElement(element);
    if (node.type == WorkspaceNode::File) {
        QString path = QDir::cleanPath(node.filePath);
        const QString relative =
            QDir(workspaceDirectory).relativeFilePath(path);
        if (!relative.startsWith(QStringLiteral("../")) &&
            relative != QStringLiteral(".."))
            path = relative;
        writer.writeAttribute(QStringLiteral("name"),
                              QDir::toNativeSeparators(path));
    } else {
        writer.writeAttribute(QStringLiteral("name"), node.name);
    }
    for (const WorkspaceNode& child : node.children)
        writeWorkspaceNode(writer, child, workspaceDirectory);
    writer.writeEndElement();
}

bool WorkspaceDocument::save(const QString& filePath, QString* errorMessage) const
{
    if (_projects.isEmpty()) {
        if (errorMessage)
            *errorMessage = QObject::tr("Workspace contains no projects.");
        return false;
    }
    QSaveFile file(filePath);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        if (errorMessage)
            *errorMessage = file.errorString();
        return false;
    }
    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.writeStartDocument(QStringLiteral("1.0"));
    writer.writeStartElement(QStringLiteral("NotepadPlus"));
    for (const WorkspaceNode& project : _projects)
        writeWorkspaceNode(writer, project, QFileInfo(filePath).absolutePath());
    writer.writeEndElement();
    writer.writeEndDocument();
    if (!file.commit()) {
        if (errorMessage)
            *errorMessage = file.errorString();
        return false;
    }
    return true;
}

void WorkspaceDocument::clear()
{
    _filePath.clear();
    _projects.clear();
}

static void appendWorkspaceFiles(const WorkspaceNode& node, QStringList* files)
{
    if (node.type == WorkspaceNode::File) {
        if (!node.filePath.isEmpty() && !files->contains(node.filePath))
            files->append(node.filePath);
        return;
    }
    for (const WorkspaceNode& child : node.children)
        appendWorkspaceFiles(child, files);
}

QStringList WorkspaceDocument::allFiles() const
{
    QStringList files;
    for (const WorkspaceNode& project : _projects)
        appendWorkspaceFiles(project, &files);
    return files;
}
