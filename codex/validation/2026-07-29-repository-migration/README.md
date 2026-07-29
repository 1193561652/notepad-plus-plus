# 仓库迁移验证

## 环境

- 分支：`qt-port`
- 基线：Notepad++ v8.4.6 Git 历史
- 编译器：MinGW-w64 x86_64 GCC 7.3
- Qt：5.12.12 Debug

## 结果

- 从空构建目录完成 CMake 配置。
- 从 `third_party/qscintilla/` 构建静态 QScintilla。
- 编入 `third_party/boostregex/` 和 `third_party/lexilla/LexUser.cxx`。
- 主程序、运行捕获工具和全部测试构建成功。
- CTest `25/25` 通过。

## 运行库

Windows 构建后会部署 Qt Core、Gui、Widgets、Network、PrintSupport、Xml、
platform/imageformat 插件及 MinGW 运行库到构建输出目录。构建产物由
`.gitignore` 排除。

## 结论

当前仓库可独立构建，不再读取旧工作区的 QScintilla、原版源码目录或诊断构建
目录。
