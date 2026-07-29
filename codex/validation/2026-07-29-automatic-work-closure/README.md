# 非人工工作自动验证

## 结果

- 主程序全量构建：通过。
- CTest：`30/30` 通过。
- 简体中文浅色 100%：通过。
- 简体中文浅色 150%：通过。
- 简体中文深色 100%：通过。
- 简体中文深色 150%：通过。

## 自动覆盖

- 通过 Mark 菜单真实入口打开 Find 对话框。
- 校验 Find 标题、5 个标签、Mark 按钮和项目查找控件资源。
- 通过 Preferences 动作进入模态对话框。
- 校验 19 个页面、自动插入、文件关联分类和静态提示中文。
- 捕获主窗口、Project Panels、Shortcut Mapper、5 个 Find 页面和 19 个
  Preferences 页面。

## 本地产物

- `./build/ui-localization-runtime-100/`
- `./build/ui-localization-runtime-150/`
- `./build/ui-localization-dark-runtime-100/`
- `./build/ui-localization-dark-runtime-150/`

这些目录属于构建产物，不作为源码知识库内容提交。
