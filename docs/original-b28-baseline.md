# Khôi phục B28 gốc — mốc owner 16,3 giây

Ngày29/10/2026, owner báo B33 khoảng17giây và chọn rõ **khôi phục B28**, sau khi
ban đầu nhắc B24. Mốc16,3giây trong lịch sử thuộc B28; B24 là bản khác.

## Trạng thái hiện tại

- Khôi phục nguyên file từ `backups/B28-before-B29-20261029/B28-source.zip`,
  gồm firmware, configuration, Keil project và default test/build scripts.
  Không thay vendor vì các file vendor đã khớp. Không xóa các tài liệu/test/script
  thử nghiệm đời sau; chúng không thuộc default build/test B28.
- **Build mới khớp hoàn toàn HEX B28 gốc**:
  `824DEF484B0028BB7029E80B02EE208D9D16C408249D28F38276B5BCA3BF9185`.
  Đây là restore đầy đủ, không chỉ lấy lại Follow/TurnOutput như B33.
- Base45/Max85, PID12/0/0,12, extraP confirmed6ms và arc28ms; OLED B28.
- Keil0Error/0Warning; Code32956, RO1856, RW440, ZI10520; end0x080088D0.
- **104.022 ARM assertions +8 parser/layout +18 receiver tests pass.**
- Chưa nạp board hoặc đo lại16,3giây trong phiên; đó là mốc owner báo trước đây.

Source/HEX B33 đã giữ tại
`backups/B33-before-original-B28-restore-20261029` trước khi overwrite.
Error log và decision history đời sau được giữ để không quên lỗi; phần ghi
current của các bản đó là lịch sử khi mâu thuẫn với lần restore này.

Baseline lưu riêng, có manifest SHA256:

- `backups/baselines-20261029/B28-owner-16p3`: B28-source.zip, HEX B28 gốc.
- `backups/baselines-20261029/B24-reference`: source-B24.zip, HEX B24; không gán
  mốc16,3giây cho B24.
- HEX tiện nạp: `build/linefollower-B28-original-16p3.hex`.

B28 gốc **không chứa** fixes recovery budget/valid±1500 của B30/B32. Các nguy cơ
đã ghi trong ERR-052/057/058 còn cần lưu ý khi phát triển từ baseline này.
Không gọi đây là bản đã giữ các fixes mới. Restore là lựa chọn owner yêu cầu.

## Hướng tối ưu sau khi xác nhận lại baseline

Không tăng Base hay giữ bánh trong luôn18%; không thêm arc80ms. Giữ cua mềm và
nét đứt làm regression trên xe. Đo lại cùng45/85/PID/pin/map trước thay đổi tiếp.

Lấy CORNER log đúng góc đang chậm để tách ba nhánh:

| Log cho thấy | Hướng thay đổi có thể thử riêng |
| --- | --- |
| TRACK ở mép lâu, correction chưa đủ hoặc bị gate | Kiểm tra cue/gain/clipping trước; chỉ chỉnh hỗ trợ góc, giữ gain cua mềm |
| TURN mất nhiều thời gian | Kiểm tra hướng và retarget trước; nếu hướng đúng mới thử duty search riêng, không tăng Base toàn map |
| Đã valid nhưng ALIGN/post-turn kéo dài | Kiểm tra điều kiện handoff/cap; không ép quay tiếp khi đã có dữ liệu line |

Mỗi lần chỉ thay một chính sách và đo lại cả thời gian lẫn số lần văng/mất line.
Command tests không dự đoán lap time; không hứa nhanh hơn16,3giây trước khi thử.
Khi cần sửa lỗi recovery đã biết, đưa từng fix riêng vào baseline và thử lại,
không gộp cùng một thay đổi tốc độ mới.
