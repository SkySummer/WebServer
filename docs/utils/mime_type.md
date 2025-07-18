# 📄 MimeType 模块

`MimeType` 模块是 HTTP 服务器的 MIME 类型管理工具，用于根据文件扩展名快速匹配标准 MIME 类型，确保响应头 `Content-Type` 的准确性，支持常见文件类型的自动识别与兼容处理。

## ✨ 模块职责

- **MIME 类型映射**：通过预定义映射表关联文件扩展名与标准 MIME 类型（如 `.html` -> `text/html`）。
- **扩展名规范化**：统一将文件扩展名转换为小写，消除大小写敏感性。
- **默认类型处理**：对未知扩展名返回 `application/octet-stream`，避免客户端解析错误。

## 📌 核心特性

- **高效查询机制**：基于哈希表（`unordered_map`）实现 O(1) 复杂度的扩展名匹配。
- **广泛类型覆盖**：支持 30+ 常见文件类型，涵盖文本、图像、音视频、字体等类别。
- **无状态设计**：通过静态方法提供功能，无需实例化即可调用。
- **扩展灵活性**：通过修改 `MIME_MAP` 可轻松添加自定义 MIME 类型。

## 📁 成员组成

| 类型/名称 | 描述 |
| ---- | ---- |
| `detail::MIME_MAP` | 静态映射表，存储扩展名到 MIME 类型的键值对（如 `{".png", "image/png"}`）。 |
| `detail::getMime` | 内部函数，根据文件路径提取扩展名并查询映射表。 |

## ⚙️ 方法概览

| 方法名称 | 功能描述 |
| ---- | ---- |
| `MimeType::get` | 接收文件路径，返回对应的 MIME 类型（如 `path/to/image.jpg` -> `image/jpeg`）。 |

## 🔄 匹配规则

1. **提取扩展名**：从文件路径中获取扩展名（如 `/static/style.CSS` -> `.css`）。
2. **小写转换**：统一转为小写（`.CSS` -> `.css`）。
3. **哈希表查询**：在预定义映射表中查找扩展名，命中则返回对应 MIME 类型。
4. **默认回退**：未命中时返回 `application/octet-stream`。

## ⚠️ 注意事项

- **无扩展名文件**：无扩展名文件（如 `README`）会返回默认类型。
- **扩展名冲突**：若同一扩展名映射多个 MIME 类型（如 `.js` 可能对应 `application/javascript` 或 `text/javascript`），需确保映射表定义符合实际需求。
- **编码一致性**：文本类 MIME 类型默认包含 `charset=UTF-8`，二进制类型（如图片）不添加字符集。

## 🔑 关键设计

- **静态映射表优化**：使用 `unordered_map` 实现高效查询，避免线性遍历开销。
- **大小写不敏感处理**：扩展名统一转为小写，适配不同操作系统和用户输入习惯。
- **模块化隔离**：通过 `detail` 命名空间隐藏实现细节，仅暴露简洁的 `get` 接口。
