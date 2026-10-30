# B34 — Thử đổi cách tính vị trí line, giữ controller B28

Owner báo B28 vừa khôi phục vẫn khoảng17giây và xác nhận45/85,12/0/0,12 như
trước; muốn đổi hướng phát triển. Kiểm tra trước sửa: source firmware bằng
snapshot B28, HEX ở máy bằng original
`824DEF484B0028BB7029E80B02EE208D9D16C408249D28F38276B5BCA3BF9185`.
Điều này xác nhận file backup/build, không đọc được Flash đang nạp trên xe hoặc
giải thích chênh lệch lap. Mốc16,3giây là owner report, không phải bảo đảm.

## Giả thuyết mới

Không đổi gain, tốc quay, floor bánh trong hoặc thời gian arc. Kiểm tra tín hiệu
đầu vào PID: B28 mask dùng ngưỡng500 nhưng centroid cộng mọi strength>=150 ở
cả8 mắt. Khi chỉ có một cụm đen mạnh, một cụm yếu tách rời ở xa vẫn kéo error
về phía đó, dù không tạo thêm bit đen. Có thể gây correction/D transient không
phù hợp với cụm đang bám. Chưa chứng minh đây là nguyên nhân chiếm phần lớn lap.

Hai log cũ CRC hợp lệ cho thấy chỉ6 trong303 valid frames bị đổi theo candidate:

| Log | Timestamp | Error B28 | Error B34 từ cùng raw |
| --- | ---: | ---: | ---: |
| B22 seq2 | 334939 | +751 | -68 |
| B22 seq2 | 334971 | -302 | +7 |
| B23 seq3 | 124774 | +343 | -27 |
| B23 seq3 | 124778 | +40 | -10 |
| B23 seq3 | 124810 | +315 | +18 |
| B23 seq3 | 124814 | +372 | -19 |

Mask các frame trên đều0x18. Đây là **recalculation**, không có ground truth vị
trí line hay dữ liệu lap B28 vừa chạy17s. Tín hiệu yếu tách rời có thể là phản
xạ/nhiễu **hoặc nhánh thật đang tới**; không được gọi mọi tín hiệu đó là nhiễu.

## Một thay đổi production

`app/line_estimator.c`: giữ nguyên normalization, polarity/calibration, deadband
150, mask500 và validity. Nếu có đúng một active group, lấy first/last bit của
group, mở rộng qua các strength>0 liền kề, dừng ở mắt strength0. Chỉ tính centroid
trong vùng đó. Giữ các weak shoulder nối với core để nội suy giữa hai mắt.

- Không chọn nhánh từ nhiều strong groups. Full-white/multi-group/full-width
  vẫn invalid; không tạo fresh error trên khoảng trắng.
- Không thêm filter theo thời gian hoặc delay; xử lý mỗi frameADC2ms như cũ.
- `strength[8]`/mask trên OLED và raw/calibration trong blackbox giữ nguyên;
  chỉ error hợp lệ có thể khác.
- Controller/gains/target45/Max85, extraP6ms, arc28ms, recovery/GAP/driver và
  watchdog của B28 không đổi. Firmware tag B34 để phân biệt log.
- Đây không phải nhận biết hình học90° sớm hơn, không có yaw/encoder/map model.
  Nếu isolated weak signal là thông tin nhìn trước thật, candidate có thể làm
  phản ứng muộn hơn. Cần thử chứ chưa được khẳng định tối ưu.

B34 dựa trên **B28 gốc**, nên known recovery limitations ERR-052/057/058 chưa
được sửa trong lần này. Không ghép các fixes đó cùng thay đổi estimator để
tránh khó phân biệt tác động; mọi invalid/Stop thật vẫn theo B28.

## Validation và công cụ đối chiếu

- B28 fail test190035: một cụm yếu rời xa kéo error của line giữa khỏi0.
- Tests mirror/both polarity giữ weak shoulder, không false-valid nhiều nhánh,
  đủ256 binary masks mỗi polarity, giữ invalid history và bounds.
- Replay raw B22 vẫn kiểm tra normalization khớp error log B28, mask/validity
  khớp toàn bộ; chỉ error ở hai frame đã xác định được đổi thành-68 và+7.
  Không sửa raw fixture hoặc bỏ assertion vì thay đổi thuật toán.
- **105.365 ARM assertions +8 parser/layout +18 receiver tests pass.** Bộ control
  và recovery B28 vẫn pass; không mô phỏng động học hoặc khẳng định WCET trên xe.
- Keil **0 Error / 0 Warning**, Code33084/RO1856/RW440/ZI10520bytes;
  HEX end0x08008950, dưới vùng log0x0800C000.
- HEX SHA256: `E729658B2394900BC904AE6AEBAEFEE76694A1C19ABC8749DF88A827485D7F95`.
- Backup actual trạng thái B28 trước thử:
  `backups/B28-original-before-B34-20261029/source-before-B34.zip` và HEX.
- Chưa nạp board, chưa đo lap B34, chưa push GitHub.

Audit offline có kiểm tra CRC/schema trước khi tính lại raw:

```powershell
python scripts/audit-line-centroid.py logs/run-20261028-160554-654293.csv logs/run-20261028-182311-164428.csv
```

Xuất `build/centroid-audit/*-centroid.csv`, không ghi đè log gốc. Tool phân biệt
raw polarity và steering direction; không coi invalid held error là vị trí mới.

## Thử A/B

Nạp `build/linefollower-B34.hex`, OLED B34, calibration/white reference lại,
giữ45/85/PID12/0/0,12. So với `build/linefollower-B28-original-16p3.hex` dưới
cùng điều kiện pin/map và điểm tính thời gian. Kiểm tra nét đứt/cua mềm rồi góc
vuông; nếu cue góc tới muộn hơn hoặc văng tăng thì quay về B28 gốc.

Mục tiêu thử là giảm correction giả trước khi mất line, không tăng tốc vòng quay
tìm line. Chỉ coi tốt hơn khi thời gian lặp lại và số lần văng/mất line cải thiện.
Chưa có dữ liệu để hứa nhanh hơn16,3s hoặc quy17s cho một nguyên nhân duy nhất.
