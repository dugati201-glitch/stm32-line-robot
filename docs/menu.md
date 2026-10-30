> **B46 hiện tại (30/10/2026)**: [Curve slowing](curve-slow-b46.md). Owner xác nhận B45 đạt **15,2s, ổn định 5/5 lượt**, Base 50 vào cua gọn hơn 55. B46 giữ nền B45, defaults 55/85, PID 12/0/0,90; giảm target theo độ lệch, chốt hướng từ mắt ngoài và giữ D qua centered ALIGN. B46 chưa đo thời gian trên xe. Các mục current cũ dưới đây là lịch sử khi có mâu thuẫn.


> **B45 hiện tại (30/10/2026)**: [Outer-eye grip](outer-grip-b45.md). Khôi phục nền B43; Base55/Max85, PID12/0/0,9; không Cruise. Mắt ngoài được cấp reserve ở TRACK, nhả khi error về tâm; carry/rejoin55, reacquire6ms. Owner báo B43 đạt15,5s, B44 mất16,3s; chưa đo B45. Các mục current cũ bên dưới là lịch sử khi mâu thuẫn.

> **B44 hiện tại (30/10/2026)**: [Chicane và Straight Cruise](chicane-cruise-b44.md). Base50 / Straight62 / Max85, PID14 / 0 / 0,9; trợ lực trong ALIGN, quay64/−22 rồi58/−40 sau20ms trắng. B43 owner đo15,5s; B44 đã test/build, chưa đo lap. Các mục current cũ bên dưới là lịch sử khi mâu thuẫn.

> **B43 swing turn (30/10/2026)**: [B43](swing-turn-b43.md). Base55/Max85, Kp12/Ki0/Kd0.9.

> **B42 active-eye (30/10/2026)**: [B42](active-eye-b42.md). Base55/Max85, Kp12/Ki0/**Kd0.10**.

> **B41 (30/10/2026)**: [B41](corner-carve-b41.md). Profile 55/85, Kp12/Ki0/Kd0.9.

> **B36 restored (30/10/2026)**: [B36 baseline](b36-restored-20261030.md).

> **B40 — Bẻ lái từ vạch góc còn nhìn thấy**: [B40](branch-entry-b40.md). Profile 55/85, Kp12/Ki0/Kd0.9.

> **B39 hiện tại (30/10/2026):** Góc gấp đã có hướng confirmed được thêm quyền lái trước loss: reserve có trọng số, tối đa15% inner reverse thay trần85/0; clear returning line nhả reserve, jitter nhỏ không làm bật/tắt. Giữ55/85,12/0/0,9, D shaping và recovery fixes. Map là reference, xe không encoder. Xem [B39](yaw-reserve-b39.md). Đã test/build, chưa có lap trên xe; các mục current cũ bên dưới là lịch sử.

> **B38 hiện tại (30/10/2026):** Sửa timeout có thể dừng khi vừa bắt lại line và budget cộng dồn qua chuỗi góc; thêm settled inner-line40ms và trợ lực góc từ frame shape đầu. Giữ55/85,12/0/0,9 và D shaping B37. Xem [so sánh Car_Line_2026 và B38](car-line-b38.md). Đã test/build, chưa đo lap trên xe; các mục current cũ bên dưới là lịch sử.

> **B37 trial hiện tại:** Owner báo B36 đạt16,11s với Base55/Max85 và Kd0,8–1 nhưng giật ở góc nối nhau. Default mới12/0/0,9,55/85; điều tiết riêng D ở vùng tâm/góc và hồi tốc nhanh chỉ khi fast exit đã yên. Xem [B37](high-kd-b37.md). Chưa có lap B37 hoặc xác nhận15s; policy cũ bên dưới là lịch sử.

> **B36 trial hiện tại:** Tăng P bổ sung riêng cho góc confirmed, giữ ramp cũ; line hẹp giữa ổn định6ms được handoff/targetBase sớm, giữ hướng nhớ và budget. Base45/Max85, PID nền12/0/0,12. Xem [B36](video-corner-b36.md). Đây là thử theo mục tiêu video, chưa có lap trên xe; policy cũ bên dưới là lịch sử.

> **B35 hiện tại:** Bỏ forward recovery arc28ms; mọi recovery đã đủ hướng/confirmation rẽ trực tiếp như chained turn. Valid vẫn PID ngay; giữ B34 estimator,45/85 và12/0/0,12. Rejoin dùng cap40 của pivot branch cũ. Xem [B35](immediate-corner-b35.md). Đã test/build, chưa thử xe; policy cũ bên dưới là lịch sử.

> **B34 trial hiện tại:** Thử cách tính vị trí line từ cụm tín hiệu nối liền, giữ nguyên controller/PID/tốc độ B28. Source và HEX B28 trước sửa đã được xác minh; owner báo khoảng17s với cùng thông số. Xem [B34](local-centroid-b34.md). Chưa có lap B34; các ghi chú current bản cũ bên dưới là lịch sử.

> **B28 hiện tại:** Base45/Max85, PID nền12/0/0,12. Tăng P từ từ ở confirmed visible corner trước khi mất line; search50 trên trắng,36 khi có tín hiệu, nhậnPD ngay khi valid. Giữ B27 arc/nét đứt/driver. Đã so ba candidate bằng command trace; chưa hardware-verified. Xem [B28](visible-corner-b28.md); policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B27 hiện tại:** Base45/Max85, PID12/0/0,12; rẽ tiến bằng PWM đang có trong28ms đầu recovery rồi mới pivot nếu còn mất line. Nhận line ngay, bảo vệ retry góc vuông; lọc hướng GAP và stagger đảo chiều để tránh both-wheel zero khi retarget. Xem [B27](rolling-corner-b27.md). Chưa hardware-verified; các policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B26 hiện tại:** Base45/Max85, PID12/0/0,12; TRACK cố định, P phản ứng ngay khi có line, rejoin giữ duty dưới cap40. Giữ recovery/góc vuông/nét đứt có giới hạn. Thêm cấu hình SWD rõ ràng và hướng dẫn lỗi nạp Keil. Xem [B26](corner-flow-b26.md) và [Keil/ST-Link](keil-download.md). **Chưa hardware-verified**; các policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B25 hiện tại:** TRACK dùng Base cố định, bỏ boost/Straight và steering floor B24. Mất line sau góc đã xác nhận chuyển TURN trực tiếp; sửa lỗi sai số giữa ±0,5 làm đảo hướng tìm line. Giữ GAP, RECENTER/post-turn và watchdog. Mặc định Base55/Max85, PID12/0/0,12; PID Setup có5 dòng. Xem [hướng dẫn B25](fixed-base-b25.md). **Chưa hardware-verified**; các chính sách cũ bên dưới là lịch sử khi mâu thuẫn.

> **B24 hiện tại:** bỏ xung BRAKE trước recovery; TURN trộn common/steering để tạo yaw ngay, valid corner có steering floor và bỏ cap32 do shape đơn thuần; reversal guard theo từng bánh. CORNER chờ cue có hướng/TURN/fault. Profile55/60/85, PID12/0/0,12. Xem [B24](speed-b24.md). Chưa hardware-verified; policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B23 hiện tại:** sửa PROBE15 theo log thật: continuation tối đa32ms khi có guide giữa ổn định, cap32%, không tăng duty khi invalid; cue rẽ/memory/recovery có ưu tiên. Profile55/60/85, PID12/0/0,12. Xem [B23](speed-b23.md). Chưa hardware-verified; policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B22 hiện tại:** hộp đen RAM → Save Flash khi dừng → Export CSV qua PB12/CH340, 9600 8N1. Vào **6. System → N2: BLACK BOX**. Controller B21 giữ nguyên; không tự tối ưu PID. Xem [hướng dẫn B22](blackbox-b22.md). Chưa hardware-verified; nội dung cũ bên dưới là lịch sử khi mâu thuẫn.

> **B21 hiện tại:** bỏ ENTRY_BRAKE khi line valid; tiếp tục forward PID và giảm PWM mượt ở cua mềm, giữ phanh khi mất line nhanh và bảo vệ chuỗi góc vuông. Profile 55/60/85, PID 12/0/0,12. Xem [B21](speed-b21.md). Các policy cũ bên dưới là lịch sử khi mâu thuẫn. Chưa hardware-verified.

> **B20 hiện tại:** 55/60/85, PID12/0/0,12; làm mượt correction có điều kiện, ENTRY BRAKE8ms khi góc đã xác nhận sau thẳng nhanh, cap35 khi xác nhận lại tâm60ms. Thêm STEER sau TIME. Xem [B20](speed-b20.md). Chưa hardware-verified; policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B19 hiện tại:** Base55 / Straight60 / Max85, thêm Straight vào PID Setup. Chỉ tăng thêm khi gần tâm đủ80ms, giữ recovery/bridge/phanh B18. Xem [B19](speed-b19.md). Chưa thử phần cứng; cấu hình cũ bên dưới là lịch sử khi mâu thuẫn.

> **B18 hiện tại:** giữ55/85, short curve bridge8ms/white4ms, tăng ramp thoát cua khi line giữa, thêm TIME sau Stop (N1: LIVE/LAST/DIAG/TIME). Xem [B18](speed-b18.md). Chưa thử phần cứng; policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B17 hiện tại:** defaults55/85, cua mềm giảm ít hơn khi ổn định, outward risk giảm sớm, corner32/pivot36/RECENTER30. Owner báo hết map, giảm khoảng1s;60/90 chậm hơn do phanh nhiều. Xem [B17](speed-b17.md). Các policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B16 hiện tại:** defaults Base50/Max80, giữ thuật toán/PID B15 và trần bonus55. Xem [profile B16](profile-b16.md). Chưa thử phần cứng; các cấu hình mặc định cũ bên dưới là lịch sử.

> **B15 hiện tại:** Base45/Max75, bonus có điều kiện lên55, cua mềm nhanh hơn và GAP giảm phần tốc dư. Xem [B15](speed-b15.md). B14 đã được owner xác nhận chạy map; B15 chưa thử phần cứng. Các policy cũ bên dưới là lịch sử khi mâu thuẫn.

> **B14 hiện tại:** giảm tốc theo valid shape lệch bên, nhớ cue trước khi mất line, thêm DIAG sau Stop để khoanh vùng. Xem [B14](diagnosis-b14.md). Quy tắc cũ bên dưới là lịch sử khi bị thay thế.

# Menu OLED hiện tại — B09

Giữ sáu mục chính theo project cũ: Run, Sensor Monitor, PID Setup, Calibration, Motor Test, System.
N1/N3 chọn dòng; N2 Enter; giữ N2 Stop và về Main Menu.

| Menu | N1 | N2 | N3 |
| --- | --- | --- | --- |
| Main Menu | Mục tiếp | Enter | Mục trước |
| PID Setup | Chọn tiếp / tăng khi edit | Edit / xác nhận | Chọn trước / giảm khi edit |
| Calibration | Auto chưa hỗ trợ | Sau sweep lấy white reference; giữ để thoát | Bắt đầu/kết thúc manual sweep |
| Motor Test | Test tiếp, dừng test cũ | Start/Stop | Test trước, dừng test cũ |
| Run, đang dừng | Nhấn ngắn: LAST RUN/Live; giữ: đổi DIR | Giữ để thoát | Start |
| Run, đang chạy | Không đổi parameter | Giữ để Stop/Back | Stop khi pressed đã debounce |
| Sensor Monitor | Giữ đổi polarity thủ công | Giữ để thoát | Chưa gán |
| System | Reset parameter | Giữ để thoát | Mở Recovery |
| Recovery | Tăng Reacq khi edit | Edit/xác nhận; giữ để thoát | Giảm Reacq khi edit |

PID Setup hiện đồng thời Kp, Ki, Kd, Base, Max. Không cuộn ẩn tốc độ.
Mặc định thử B09 Kp=12, Ki=0, Kd=0.12, Base=45%, Max=75%; xem [cruise-gap-b09.md](cruise-gap-b09.md).
Base là target cruise trên đường thẳng/cua nhẹ; Max chỉ cap từng bánh, không tự boost lên Max.
Base/Max chỉnh mỗi bước 5 điểm phần trăm PWM, Base không vượt Max; giá trị có tác dụng trong RUN.
Parameter hiện RAM-only, Reset không giữ giá trị đã chỉnh.

System → N3: Recovery hiển thị Advance DISABLED. B09 không chạy tiến mù theo geometry.
Chỉ chỉnh Reacq mặc định 12 ms, phạm vi 4..40 ms, bước 2 ms.
Motor Test vẫn 20% PWM, timeout 2 giây, không tự chạy sau boot.

Calibration: N3 sweep start/end → đặt cả 8 mắt trên trắng → N2 lấy white reference.
Start yêu cầu calibration/reference hợp lệ, line valid, ADC mới và OLED hoạt động.
Mode Run gồm READY, TRACK, GAP, BRAKE, TURN/SEARCH, PROBE, RECENTER, FAULT.
B trên Run/LAST RUN là số full brake pulse đã yêu cầu; không là lực phanh hoặc tốc độ đo được.
Run giữ V/T/label (common PWM request/target và nguyên nhân BASE/CURV/GAP/REC/BRK/PRB/OFF).
BST/WAIT là enum legacy, controller B09 không phát chúng.
LAST RUN giữ cả V/T/label, mode viết ngắn TRK/TRN/REC/BRK; giữ error/mask/PWM/SAT/B/F.
Nút điều khiển không đổi. Xem [cruise-gap-b09.md](cruise-gap-b09.md).
