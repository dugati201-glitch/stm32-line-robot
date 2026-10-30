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

> **Hiện tại: B28 gốc đã khôi phục theo lựa chọn owner.** Build mới khớp hoàn toàn HEX B28 đã lưu; mốc16,3s là kết quả owner trước đây, chưa đo lại. Đây không phải B33 ghép fixes và không chứa sửa recovery B30/B32. Xem [baseline B28](original-b28-baseline.md).

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

> **Cập nhật B09:** Base cruise và guide-memory GAP theo lựa chọn owner, xem [cruise-gap-b09.md](cruise-gap-b09.md). Bước tiếp: thử đoạn liền→nét đứt→cua nhẹ, sau đó 90° hai phía; không yêu cầu đọc OLED lúc chạy. Test logic không là xác nhận động lực học; nội dung phiên bản cũ bên dưới là lịch sử.

# Roadmap triển khai và kiểm chứng

## Trạng thái hiện tại

Đã triển khai source cho timebase, TB6612 output, ADC/DMA frame 2 ms,
buttons EXTI/NVIC, OLED và manual calibration/line estimator.
Application đã có menu cũ, Motor Test, PD/PID chu kỳ frame 2 ms, acceleration ramp, RUN và bounded gap/recovery. Owner xác nhận calibration và Motor Test; polarity mặc định đã sửa theo phản hồi. Owner xác nhận RUN ở Base 20% bám vòng tròn/cua nhẹ, chưa qua cua 90°. Owner xác nhận B03 cải thiện cua/gap, nhưng cắt chéo chuỗi góc vuông. B04 thêm ADVANCE/ALIGN, speed UI đầy đủ và Corner Timing theo geometry 16 cm/40 cm; chưa thử B04 trên xe. Auto Calibration chưa triển khai.
Build và emulation test đã pass; calibration/Motor Test có xác nhận owner, các tiêu chí RUN/timing bên dưới vẫn cần đo trên xe.
Xem [bringup.md](bringup.md) và ghi lỗi/kết quả vào [error-log.md](error-log.md).

## Các mốc triển khai

| Thứ tự | Phạm vi | Bằng chứng hoàn thành |
| --- | --- | --- |
| 1 | Xác nhận oscillator, LED, sensor level, button polarity và OLED model/address | Ghi kết quả đo, cập nhật tài liệu phần cứng |
| 2 | TB6612 GPIO/TIM4 driver với stop mode rõ ràng | Thử riêng từng bánh; xác nhận forward polarity và hành vi stop |
| 3 | ADC/DMA frame capture và sensor monitor | Tám channel ổn định; kiểm chứng cơ chế công bố frame đầy đủ |
| 4 | Manual calibration và line estimation | Đủ contrast ở từng channel; error đơn điệu và đúng dấu |
| 5 | PD chu kỳ cố định ở duty thấp | Đo period, execution time, jitter; xác nhận đúng chiều sửa lái |
| 6 | Acceleration ramp và target duty theo tracking quality | Lặp lại thử tăng tốc đường thẳng, không mất line |
| 7 | Gap crossing và recovery có giới hạn | Thử lặp lại nét đứt, off-track recovery và timeout |
| 8 | Nút nhấn EXTI/NVIC, debounce state machine, OLED menu và chỉnh parameter | Mỗi lần nhấn chỉ tạo một event; nhấn giữ/nhả đúng; bounce không gây interrupt starvation; OLED/nút không làm trễ control; stop hoạt động ở mọi mode |
| 9 | Junction navigation và route memory nếu cần | Chốt yêu cầu và chiến lược nhận diện riêng trước khi triển khai |

Triển khai từng mốc; ghi firmware revision và điều kiện thử phần cứng.
Đọc [hardware.md](hardware.md) trước khi cấp phát peripheral pin.
