# B40 — Bẻ lái từ vạch góc còn nhìn thấy

Ngày 30/10/2026. Giữ Base55/Max85, Kp12/Ki0/Kd0,9. Đây là bản thử trên xe,
chưa có kết quả lap B40 hoặc xác nhận giảm overshoot.

## Bằng chứng từ log B39

`logs/run-20261030-143151-193568.csv`: parser kiểm lại đủ 160 records,
CRC32 `1CCA0436`, cửa sổ 632 ms, không fault. Summary khớp CSV.

| ms | Mask | Trạng thái | L/R PWM % |
| --- | --- | --- | --- |
| 32538 | 98 | PROBE, invalid | 55,6 / 51,8 |
| 32540 | F8 | TRACK, error 1,506 pitch, D=0 | 73,4 / 34,3 |
| 32544 | F8 | TRACK/CORNER | 84,0 / 24,3 |
| 32548 | F8 | TRACK/CORNER, error 1,646 pitch | 85,0 / 18,6 |
| 32552 | B8 | TURN | 36 / −36 |
| 32556–32856 | 00 | TURN, không thấy line | 50 / −50 |

Không có BRAKE hoặc cả hai wheel command bằng 0 trong cửa sổ này. TURN kéo
dài ít nhất 304 ms đến hết cửa sổ; chưa biết lúc nào bắt lại line. Không được
gọi 304 ms là toàn bộ thời gian cua. PWM không đo vận tốc hoặc quán tính.

Source xóa D sau mọi loss, kể cả PROBE 2 ms ngay trước F8. Broad centroid
chỉ khoảng 1,5–1,65 pitch nên B39 reserve theo error 1,5→2,5 rất nhỏ. Bánh trong
vẫn tiến đáng kể trước loss. Đây là hai hạn chế command law có bằng chứng;
log không đủ kết luận motor/slip hoặc thời điểm ảnh video tương ứng chính xác.

## Thay đổi

1. Giữ derivative history qua PROBE có tín hiệu tối đa 4 ms **chỉ khi frame
   trở lại là directional broad shape**. Tính D với thời gian gồm cả khoảng
   gián đoạn, không chia bước nhảy cho riêng 2 ms. GAP trắng, loss dài và
   recovery vẫn xử lý như cũ. D shaping B37 còn nguyên.
2. Broad shape valid đã confirmed 6 ms, cùng hướng pending/drive, error cùng
   phía ít nhất 1 pitch, chưa trở về tâm rõ: thêm proportional branch bias
   `side * Kp * duty_weight * clamp(2.5 - abs(error), 0, 1.5)`.
   Duty weight dùng min(speed,target), tăng từ 35 tới 45%. Ở error1,5/Kp12/
   duty45+, bias là 12 điểm PWM correction. Đây là hằng số thử theo pitch,
   không phải quỹ đạo hình học tính từ map hay góc yaw đã đo.
3. Bias mất ngay khi shape thu hẹp/đổi phía, raw return rõ hoặc D hướng vào
   vượt ngưỡng. Low Base20/30 không có bias. Zero gains không tự tạo lệnh quay.
   Antiwindup tính cả bias; giới hạn wheel/Max/reverse B39 vẫn giữ.

Không thêm dwell phanh hai bánh, timed advance, max-speed scan, tăng Base,
đếm waypoint hoặc Stop theo map. Giữ sensor validity, recovery timeout/new-corner
fixes B38, nét đứt, motor reversal guard, ADC, menu, Flash và SWD.
Ở một số mẫu, bánh trong có thể nhận 0 hoặc âm theo giới hạn hiện có; không
hứa hai bánh luôn tiến hay motor thật không bị khựng.

## Kiểm tra và thử xe

- Backup: `backups/B39-before-B40-20261030`, source ZIP, SHA256 và B39 HEX.
- New regression fail trên B39 tại 250018 (D bị xóa), pass B40: 60 assertions.
  Chuỗi dựa trên log có các frame 2 ms tổng hợp; không là replay vật lý hay
  tái dựng chính xác những sample không được ghi trong CSV 4 ms.
- Integration: 106137 ARM assertions; standalone corner94/highKd4955/
  recovery541/yaw1840/branch60. Python parser/layout8 và receiver18 đều pass.
- Các test số học cũ ở confirmed broad corner cập nhật output có bias; giữ
  kiểm tra soft curve, returning line, low Base, bounds, timeout và watchdog.
- Keil: 0 errors, 0 warnings; Code34624, RO1856, RW440, ZI10536.
  Image kết thúc 0x08008F54, không chạm vùng log Flash 0x0800C000.

Nạp `build/linefollower-B40.hex`, kiểm tra OLED B40. Giữ profile như log B39
55/85,12/0/0,9 để so sánh. Đo riêng góc đầu sau nét đứt và cụm vuông liên tục,
cùng pin/calibration/map; xuất CORNER log mới và video nếu còn đi dư.
Mục tiêu là giảm nhu cầu TURN sau góc, không chỉ tăng PWM lúc đã mất line.
