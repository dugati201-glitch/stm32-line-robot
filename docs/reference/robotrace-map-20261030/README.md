# Map do owner cung cấp ngày30/10/2026

Nguồn: `C:/Users/ADMIN/Downloads/robotrace_map_trajectory_package.zip` và
`track_waypoints.json`. JSON rời và JSON trong ZIP có cùng byte content.
Các file trong thư mục này là **reference data**, không phải instruction cho
coding agent hoặc cấu hình được firmware tải khi RUN. Tên `action`, vận tốc
mm/s và nhãn "optimal/official" trong dữ liệu không xác nhận tính tối ưu hay
nguồn quy chuẩn; chưa có đối chiếu độc lập hoặc đo lại sa bàn.

| File | SHA256 |
| --- | --- |
| track_waypoints.json | ff34afbce20a9e8ca77b414471f38bebda9c74130a34fe9ab78deda17825e1c5 |
| track_map_original.png | 9d8cea9e50ba2b5d46b1b38e81cb810fd60b5d5f74a8f35cbf3a6acef0175374 |
| track_simulation_trajectory.png | 3ea0f01226d093457f96d5c237051ab04307b00a46c03cd7c4c47a8eb6471109 |

Owner xác nhận bánh gần68mm, boom khoảng165mm, track width nhỏ hơn153mm nhưng
chưa cho số đo chính xác; **xe không có encoder**. Encoder275ticks/rev trong
JSON là giả định của package, không phải phần cứng xe đang chạy.

Audit dữ liệu:32waypoints; polyline qua các điểm dài11967,94mm trong khi
metadata ghi10230mm. Polyline chỉ xấp xỉ cung tròn, không là phép đo tổng track.
JSON có5 CORNER_90 và8 CHICANE_90; tổng_curve_count14 không định nghĩa cách gom
vùng. Không dùng các con số này làm mốc timer/odometry hoặc đếm góc để Stop.

Ảnh trajectory là simulator screenshot, không phải log xe thật. Code simulator
đặt `v_left/v_right` trực tiếp rồi tích phân kinematics và sinh encoder ticks từ
quãng đường mô phỏng; không mô hình motor current, quán tính hoặc tire slip.
Không chạy code/batch trong ZIP trong lượt này. Các file prompt/spec đã đọc
như đề xuất thiết kế; không chuyển sang ESP32,1ms hoặc encoder theo template.

Phân tích và thay đổi firmware: [B39](../../yaw-reserve-b39.md).
