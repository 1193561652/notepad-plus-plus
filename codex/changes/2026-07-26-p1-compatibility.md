# 2026-07-26 P1 兼容开发

## 主要变更

- Finder 增加来源分组、匹配样式、文件/项目搜索进度与取消。
- Mark 页启用 Copy Marked Text。
- 文件监控增加目录重命名识别、权限同步和定时轮询。
- 打印应用页眉页脚变量、颜色模式和行号配置。
- 补齐整数、小数点、小数逗号、随机和矩形列排序。
- Column Editor 补齐进制、前导零和重复选项。
- 集成 v8.4.6 LexUser，扩展 QScintilla 关键词集合和必要 lexlib 接口。
- UDL 支持 28 个列表、目录语言文件、nesting 和保守原位写回。
- 新增 UDL 设计器和保存搜索宏 type 3 回放。
- 注册 P1 排序和 UDL 对话框的原版 command ID。

## 验证

```text
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
```

结果：构建成功，CTest 18/18 通过。

未执行主程序启动、截图、人工 UI 操作、外部文件提示和真实打印验证。

## 运行验证补充

- 后续获得用户授权，P1 专用运行验证最终退出码 0。
- 修复全文排序处理末尾占位行导致的额外空行。
- 修复 Finder 默认文本与匹配高亮标签叠绘。
- 打印实现位于 `src/ScintillaComponent/Printer.*`，并使用生产组件生成
  PDF 完成页眉页脚、变量、页码和行号验证。
- UDL 隔离写回、外部修改/重命名/删除和只读状态均验证通过。
