# Kết quả benchmark

Seed: `20261001`; warm-up: `1`; số lần đo: `5`.

| Dataset | Tác vụ | Tối ưu (median ms) | Baseline (median ms) | Tăng tốc |
|---|---|---:|---:|---:|
| 10,000 | mc1_exact_id | 0.530 | 28.486 | 53.76x |
| 10,000 | mc2_priority | 1.361 | 6.526 | 4.79x |
| 100,000 | mc1_exact_id | 0.966 | 270.590 | 280.06x |
| 100,000 | mc2_priority | 14.648 | 75.287 | 5.14x |

Dữ liệu thô: [raw.csv](raw.csv). Thời gian nạp CSV và dựng chỉ mục được ghi riêng với `phase=setup`.
