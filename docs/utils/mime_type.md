# 🗂️ MimeType 模块

`MimeType` 模块用于根据文件后缀名推断对应的 MIME 类型，在响应 HTTP 静态资源时自动设置正确的 `Content-Type`，确保浏览器能够正确识别和显示内容。

## ✨ 模块功能

- 根据文件路径自动匹配 MIME 类型
- 默认使用 UTF-8 编码处理文本资源
- 对于未知类型返回 `application/octet-stream`
- 内置常用静态资源 MIME 映射表，支持图片、视频、字体、压缩包等

## 📌 主要特性

- **轻量快速**：使用哈希表查找，效率高
- **封装良好**：对外只提供一个静态接口 `get`
- **高度可复用**：可在静态文件模块、错误页面模块等多处统一调用
- **编码兼容性好**：文本类型统一加上 `charset=UTF-8`，避免浏览器乱码
- **纯头文件实现**：采用 header-only 方式，方便集成与部署。
- **便于扩展**：新增类型仅需修改映射表，无需更改逻辑代码

## 🛠️ 使用说明

只需传入文件路径，即可获取对应 MIME 类型字符串。
