# 📦 StaticFile 模块

`StaticFile` 模块是 WebServer 的静态资源处理组件，负责响应 HTML、CSS、JS、图片等文件请求，并在访问异常时生成标准化错误页面。支持缓存静态资源，大幅提升文件响应性能。

## ✨ 模块功能

- 响应用户请求的本地静态资源文件。
- 自动推断 MIME 类型，设置正确的 `Content-Type`。
- 校验路径合法性，防止目录穿越。
- 缓存静态资源内容，支持缓存命中、失效检测。
- 根据错误状态码返回标准错误页面。

## 📌 主要特性

- **安全性**：规范化路径，防止非法访问。
- **错误处理**：内置常见 HTTP 错误页面模板。
- **缓存加速**：缓存热点资源，减少磁盘 I/O，提高响应速度。
- **可配置**：支持自定义资源根目录。
- **日志记录**：记录客户端访问信息与缓存命中情况，辅助调试。

## 🛠️ 使用说明

用于处理 `GET` 请求中的文件访问，判断文件是否合法、是否存在，并生成标准 HTTP 响应返回客户端。访问路径会统一规范化，并在首次访问时缓存资源内容。后续重复访问将命中缓存，加速响应。

当文件发生变更时，缓存会失效并自动重新加载。

## 📁 错误状态

当前模块支持以下常见错误状态码：

- `400 Bad Request`
- `403 Forbidden`
- `404 Not Found`
- `405 Method Not Allowed`
- `500 Internal Server Error`
- `502 Bad Gateway`

可根据需求扩展或新增其他错误状态码，模块会自动处理并返回相应的错误页面。
