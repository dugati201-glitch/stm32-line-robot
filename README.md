> **B46 hiện tại (30/10/2026)**: [Curve slowing](docs/curve-slow-b46.md). Owner xác nhận B45 đạt **15,2s, ổn định 5/5 lượt**, Base 50 vào cua gọn hơn 55. B46 giữ nền B45, defaults 55/85, PID 12/0/0,90; giảm target theo độ lệch, chốt hướng từ mắt ngoài và giữ D qua centered ALIGN. B46 chưa đo thời gian trên xe. Các mục current cũ dưới đây là lịch sử khi có mâu thuẫn.


> **B45 hiện tại (30/10/2026)**: [Outer-eye grip](docs/outer-grip-b45.md). Khôi phục nền B43; Base55/Max85, PID12/0/0,9; không Cruise. Mắt ngoài được cấp reserve ở TRACK, nhả khi error về tâm; carry/rejoin55, reacquire6ms. Owner báo B43 đạt15,5s, B44 mất16,3s; chưa đo B45. Các mục current cũ bên dưới là lịch sử khi mâu thuẫn.

> **B44 hiện tại (30/10/2026)**: [Chicane và Straight Cruise](docs/chicane-cruise-b44.md). Base50 / Straight62 / Max85, PID14 / 0 / 0,9; trợ lực trong ALIGN, quay64/−22 rồi58/−40 sau20ms trắng. B43 owner đo15,5s; B44 đã test/build, chưa đo lap. Các mục current cũ bên dưới là lịch sử khi mâu thuẫn.

> **B43 swing turn (30/10/2026)**: [B43](docs/swing-turn-b43.md). Base55/Max85, Kp12/Ki0/Kd0.9.

> **B42 active-eye (30/10/2026)**: [B42](docs/active-eye-b42.md). Base55/Max85, Kp12/Ki0/**Kd0.10**.

> **B41 (30/10/2026)**: [B41](docs/corner-carve-b41.md). Profile 55/85, Kp12/Ki0/Kd0.9.

> **B36 restored (30/10/2026)**: [B36 baseline](docs/b36-restored-20261030.md).

> **B40 — Bẻ lái từ vạch góc còn nhìn thấy**: [B40](docs/branch-entry-b40.md). Profile 55/85, Kp12/Ki0/Kd0.9.

> **B39 hiện tại (30/10/2026):** Góc gấp đã có hướng confirmed được thêm quyền lái trước loss: reserve có trọng số, tối đa15% inner reverse thay trần85/0; clear returning line nhả reserve, jitter nhỏ không làm bật/tắt. Giữ55/85,12/0/0,9, D shaping và recovery fixes. Map là reference, xe không encoder. Xem [B39](docs/yaw-reserve-b39.md). Đã test/build, chưa có lap trên xe; các mục current cũ bên dưới là lịch sử.

> **B38 hiện tại (30/10/2026):** Sửa timeout có thể dừng khi vừa bắt lại line và budget cộng dồn qua chuỗi góc; thêm settled inner-line40ms và trợ lực góc từ frame shape đầu. Giữ55/85,12/0/0,9 và D shaping B37. Xem [so sánh Car_Line_2026 và B38](docs/car-line-b38.md). Đã test/build, chưa đo lap trên xe; các mục current cũ bên dưới là lịch sử.

> **B37 trial hiện tại:** Owner báo B36 đạt16,11s với Base55/Max85 và Kd0,8–1 nhưng giật ở góc nối nhau. Default mới12/0/0,9,55/85; điều tiết riêng D ở vùng tâm/góc và hồi tốc nhanh chỉ khi fast exit đã yên. Xem [B37](docs/high-kd-b37.md). Chưa có lap B37 hoặc xác nhận15s; policy cũ bên dưới là lịch sử.

> **B36 trial hiện tại:** Tăng P bổ sung riêng cho góc confirmed, giữ ramp cũ; line hẹp giữa ổn định6ms được handoff/targetBase sớm, giữ hướng nhớ và budget. Base45/Max85, PID nền12/0/0,12. Xem [B36](docs/video-corner-b36.md). Đây là thử theo mục tiêu video, chưa có lap trên xe; policy cũ bên dưới là lịch sử.

> **B35 hiện tại:** Bỏ forward recovery arc28ms; mọi recovery đã đủ hướng/confirmation rẽ trực tiếp như chained turn. Valid vẫn PID ngay; giữ B34 estimator,45/85 và12/0/0,12. Rejoin dùng cap40 của pivot branch cũ. Xem [B35](docs/immediate-corner-b35.md). Đã test/build, chưa thử xe; policy cũ bên dưới là lịch sử.

> **B34 trial hiện tại:** Thử cách tính vị trí line từ cụm tín hiệu nối liền, giữ nguyên controller/PID/tốc độ B28. Source và HEX B28 trước sửa đã được xác minh; owner báo khoảng17s với cùng thông số. Xem [B34](docs/local-centroid-b34.md). Chưa có lap B34; các ghi chú current bản cũ bên dưới là lịch sử.

> **Hiện tại: B28 gốc đã khôi phục theo lựa chọn owner.** Build mới khớp hoàn toàn HEX B28 đã lưu; mốc16,3s là kết quả owner trước đây, chưa đo lại. Đây không phải B33 ghép fixes và không chứa sửa recovery B30/B32. Xem [baseline B28](docs/original-b28-baseline.md).

> **B28 hiện tại:** Base45/Max85, PID nền12/0/0,12. Tăng P từ từ ở confirmed visible corner trước khi mất line; search50 trên trắng,36 khi có tín hiệu, nhậnPD ngay khi valid. Giữ B27 arc/nét đứt/driver. Đã so ba candidate bằng command trace; chưa hardware-verified. Xem [B28](docs/visible-corner-b28.md); policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B27 hiện tại:** Base45/Max85, PID12/0/0,12; rẽ tiến bằng PWM đang có trong28ms đầu recovery rồi mới pivot nếu còn mất line. Nhận line ngay, bảo vệ retry góc vuông; lọc hướng GAP và stagger đảo chiều để tránh both-wheel zero khi retarget. Xem [B27](docs/rolling-corner-b27.md). Chưa hardware-verified; các policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B26 hiện tại:** Base45/Max85, PID12/0/0,12; TRACK cố định, P phản ứng ngay khi có line, rejoin giữ duty dưới cap40. Giữ recovery/góc vuông/nét đứt có giới hạn. Thêm cấu hình SWD rõ ràng và hướng dẫn lỗi nạp Keil. Xem [B26](docs/corner-flow-b26.md) và [Keil/ST-Link](docs/keil-download.md). **Chưa hardware-verified**; các policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B25 hiện tại:** TRACK dùng Base cố định, bỏ boost/Straight và steering floor B24. Mất line sau góc đã xác nhận chuyển TURN trực tiếp; sửa lỗi sai số giữa ±0,5 làm đảo hướng tìm line. Giữ GAP, RECENTER/post-turn và watchdog. Mặc định Base55/Max85, PID12/0/0,12; PID Setup có5 dòng. Xem [hướng dẫn B25](docs/fixed-base-b25.md). **Chưa hardware-verified**; các chính sách cũ bên dưới là lịch sử khi mâu thuẫn.

> **B24 hiện tại:** bỏ xung BRAKE trước recovery; TURN trộn common/steering để tạo yaw ngay, valid corner có steering floor và bỏ cap32 do shape đơn thuần; reversal guard theo từng bánh. CORNER chờ cue có hướng/TURN/fault. Profile55/60/85, PID12/0/0,12. Xem [B24](docs/speed-b24.md). Chưa hardware-verified; policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B23 hiện tại:** sửa PROBE15 theo log thật: continuation tối đa32ms khi có guide giữa ổn định, cap32%, không tăng duty khi invalid; cue rẽ/memory/recovery có ưu tiên. Profile55/60/85, PID12/0/0,12. Xem [B23](docs/speed-b23.md). Chưa hardware-verified; policy cũ bên dưới là lịch sử khi mâu thuẫn.

> Nhận log không dùng Hercules: chạy `scripts/receive-blackbox.ps1`, chọn COM rồi bấm Export trên OLED; tự lưu CSV và kiểm tra CRC. Xem [hướng dẫn nhận log](docs/receive-log.md). Dùng với firmware B22 hiện tại, không cần nạp lại.

> **B22 hiện tại:** hộp đen RAM → Save Flash khi dừng → Export CSV qua PB12/CH340, 9600 8N1. Vào **6. System → N2: BLACK BOX**. Controller B21 giữ nguyên; không tự tối ưu PID. Xem [hướng dẫn B22](docs/blackbox-b22.md). Chưa hardware-verified; nội dung cũ bên dưới là lịch sử khi mâu thuẫn.

> **B21 hiện tại:** bỏ ENTRY_BRAKE khi line valid; tiếp tục forward PID và giảm PWM mượt ở cua mềm, giữ phanh khi mất line nhanh và bảo vệ chuỗi góc vuông. Profile 55/60/85, PID 12/0/0,12. Xem [B21](docs/speed-b21.md). Các policy cũ bên dưới là lịch sử khi mâu thuẫn. Chưa hardware-verified.

> **B20 hiện tại:** 55/60/85, PID12/0/0,12; làm mượt correction có điều kiện, ENTRY BRAKE8ms khi góc đã xác nhận sau thẳng nhanh, cap35 khi xác nhận lại tâm60ms. Thêm STEER sau TIME. Xem [B20](docs/speed-b20.md). Chưa hardware-verified; policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B19 hiện tại:** Base55 / Straight60 / Max85, thêm Straight vào PID Setup. Chỉ tăng thêm khi gần tâm đủ80ms, giữ recovery/bridge/phanh B18. Xem [B19](docs/speed-b19.md). Chưa thử phần cứng; cấu hình cũ bên dưới là lịch sử khi mâu thuẫn.

> **B18 hiện tại:** giữ55/85, short curve bridge8ms/white4ms, tăng ramp thoát cua khi line giữa, thêm TIME sau Stop (N1: LIVE/LAST/DIAG/TIME). Xem [B18](docs/speed-b18.md). Chưa thử phần cứng; policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B17 hiện tại:** defaults55/85, cua mềm giảm ít hơn khi ổn định, outward risk giảm sớm, corner32/pivot36/RECENTER30. Owner báo hết map, giảm khoảng1s;60/90 chậm hơn do phanh nhiều. Xem [B17](docs/speed-b17.md). Các policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B16 hiện tại:** defaults Base50/Max80, giữ thuật toán/PID B15 và trần bonus55. Xem [profile B16](docs/profile-b16.md). Chưa thử phần cứng; các cấu hình mặc định cũ bên dưới là lịch sử.

> **B15 hiện tại:** Base45/Max75, bonus có điều kiện lên55, cua mềm nhanh hơn và GAP giảm phần tốc dư. Xem [B15](docs/speed-b15.md). B14 đã được owner xác nhận chạy map; B15 chưa thử phần cứng. Các policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B14 hiện tại:** giảm tốc theo valid shape lệch bên, nhớ cue trước khi mất line, thêm DIAG sau Stop để khoanh vùng. Xem [B14](docs/diagnosis-b14.md). Quy tắc cũ bên dưới là lịch sử khi bị thay thế.

# STM32 Line Follower — B09 / SPL

STM32F103C8T6, line analog 8 mắt, TB6612, hai GA25 620 RPM không encoder,
OLED I2C và ba nút EXTI/NVIC. Sửa bằng VS Code, build bằng Keil ARMCC 5.

## Trạng thái hiện tại

B09 chạy Base cruise, bỏ tự boost/cooldown; giữ lệnh L/R đáng tin qua nét đứt tối đa 300 ms.
Có line valid thì forward PD lái; không dùng ADVANCE/geometry timing hoặc ép quay theo map.
Giữ calibration + white reference, ADC/DMA 2 ms, OLED menu, Motor Test và watchdog.
Mặc định thử Base 45%, Max 75% (trần từng bánh), Kp 12, Ki 0, Kd 0.12; chỉnh trên OLED, chưa lưu Flash.
Owner báo nét đứt/cua nhẹ khựng và chọn chạy Base cố định; B09 cần thử lại trên xe thực.

Đọc [hướng dẫn B09](docs/cruise-gap-b09.md), [menu](docs/menu.md) và
[nhật ký lỗi](docs/error-log.md) trước khi chạy hoặc sửa firmware.
Không coi test giả lập là xác nhận xe chạy tốt trên phần cứng.

## Build và nạp

- VS Code: Ctrl+Shift+B gọi `scripts/build.ps1`; hoặc mở `linefollower.uvprojx` trong Keil.
- CLI không cần mở uVision: `powershell -File scripts/build-headless.ps1`.
- Cả hai dùng ARMCC 5, target Cortex-M3, Flash 64 KiB và RAM 20 KiB.
- Nạp `build/linefollower-B09.hex` hoặc Download AXF từ Keil; kiểm tra OLED hiện B09.
- Build task không tự flash. Source/artifact B08 giữ tại `backups/B08-before-B09-20261024`.

Sau nạp: Calibration N3 start/end → cả 8 mắt lên trắng → N2 WHITE REF.
N2 vào Run, N3 Start/Stop. Kiểm tra hướng sửa lái ở PWM thấp trước khi tăng tốc.
Sau Stop nhấn ngắn N1 để xem LAST RUN (mode/V/T/reason/error/mask/PWM/B trước dừng).

## Cấu trúc

| Folder | Vai trò |
| --- | --- |
| `app/` | Estimator, controller, supervisor và menu |
| `bsp/` | ADC/DMA, TB6612, EXTI buttons, OLED, timebase |
| `config/` | Board pin/configuration |
| `startup/`, `vendor/` | Startup, SPL và CMSIS; giữ license/provenance |
| `scripts/` | Build và emulation tests |
| `docs/` | Kiến trúc, phần cứng, hướng dẫn và error log |
| `tests/` | Regression tests; không được link vào firmware |

Xem [mục lục tài liệu](docs/README.md). STBY nối trực tiếp 3V3, không có STBY GPIO.
