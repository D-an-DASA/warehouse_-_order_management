async function createRevenueChart() {

    try {
        // Lấy dữ liệu từ C++ server
        const response = await fetch("http://localhost:8080/data");

        // Chuyển JSON thành JavaScript object
        const data = await response.json();

        // Tách dữ liệu cho Chart.js
        const labels = data.map(item => item.category);
        const values = data.map(item => item.value);

        // Tạo biểu đồ
        const canvas = document.getElementById("revenueChart");

        new Chart(canvas, {
            type: "bar",

            data: {
                labels: labels,

                datasets: [{
                    label: "Doanh thu",
                    data: values
                }]
            },

            options: {
                responsive: true,

                plugins: {
                    title: {
                        display: true,
                        text: "Doanh thu theo chi nhánh"
                    }
                },

                scales: {
                    y: {
                        beginAtZero: true
                    }
                }
            }
        });

    } catch (error) {
        console.error("Cannot get data from C++:", error);
    }
}