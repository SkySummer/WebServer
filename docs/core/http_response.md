# 📤 HttpResponse 模块

`HttpResponse` 模块是 HTTP 服务器的响应构建核心，负责生成符合 HTTP 协议的响应内容，支持自定义状态码、头部字段及正文内容，并提供标准化错误页面生成功能，简化服务器响应逻辑。

## ✨ 模块职责

- **响应构建**：组装 HTTP 状态行、头部字段和正文，生成完整响应字符串。
- **错误处理**：通过预设模板快速生成常见 HTTP 错误页面（如 404、500）。
- **内容管理**：自动计算 `Content-Length`，确保响应头部与正文一致性。
- **灵活配置**：支持链式调用设置状态、内容类型及自定义头部。

## 📌 核心特性

- **链式调用设计**：通过返回 `HttpResponse&` 支持流畅接口（如 `.setStatus().setContentType()`）。
- **错误响应模板化**：内置 HTML 错误页面模板，支持动态填充状态码、描述及提示信息。
- **自动化头部管理**：自动添加 `Content-Length` 和 `Connection: close` 头部字段。
- **多错误码支持**：覆盖常见 HTTP 错误码（400/403/404/405/500/502），提供友好错误提示。

## 📁 成员组成

| 类型/名称 | 描述 |
| ---- | ---- |
| `std::string status_` | HTTP 状态行（如 `200 OK`、`404 Not Found`）。 |
| `std::string body_` | 响应正文内容，支持文本、HTML 或二进制数据。 |
| `std::map<std::string, std::string> headers_` | HTTP 头部键值对，存储如 `Content-Type`、`Location` 等字段。 |

## ⚙️ 方法概览

| 方法名称 | 功能描述 |
| ---- | ---- |
| `setStatus` | 设置 HTTP 状态码及描述（如 `setStatus("404 Not Found")`）。 |
| `setContentType` | 指定 `Content-Type` 头部（如 `text/html`、`application/json`）。 |
| `setBody` | 设置响应正文内容，支持任意字符串格式。 |
| `addHeader` | 添加自定义 HTTP 头部（如重定向 `Location: /new-path`）。 |
| `build` | 生成完整 HTTP 响应字符串，包含状态行、头部及正文。 |
| `buildErrorResponse` | 静态方法，根据错误码生成标准化错误响应（含 HTML 页面）。 |

## 🔄 工作流程

1. **初始化响应对象**
   - 创建 `HttpResponse` 实例，默认状态为 `200 OK`，无正文和自定义头部。
2. **配置响应参数**
   - 链式调用方法设置状态、内容类型、正文及自定义头部（如重定向或缓存控制）。
3. **生成响应内容**
   - 调用 `build` 方法，自动计算 `Content-Length`，拼接状态行、头部和正文。
   - 默认添加 `Connection: close`，指示客户端关闭连接。
4. **错误响应处理**
   - 调用 `buildErrorResponse(404)` 生成包含 HTML 的错误页面，状态码与描述动态填充。
5. **输出响应**
   - 将生成的响应字符串发送至客户端，完成请求-响应周期。

## 🔑 关键设计

- **链式方法设计**：提升代码可读性，简化复杂响应的配置过程。
- **错误页面统一化**：通过模板减少重复代码，确保错误格式一致。
- **自动化内容长度**：避免手动计算 `Content-Length`，降低出错风险。
- **轻量级对象**：每个 `HttpResponse` 实例独立，天然支持多线程并发构建。
