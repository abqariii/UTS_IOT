// Function to fetch data from MongoDB via the Flask API
async function fetchSensorData() {
    try {
        const response = await fetch('/get_data');
        const data = await response.json();

        // Extract data for temperature, humidity, and timestamps
        const temperatureData = data.map(item => item.nilaisuhuhumid.map(val => val.suhu)).flat();
        const humidityData = data.map(item => item.nilaisuhuhumid.map(val => val.humid)).flat();
        const timestamps = data.map(item => item.nilaisuhuhumid.map(val => new Date(val.timestamp).toLocaleTimeString())).flat();

        // Initialize the chart
        const ctx = document.getElementById("temperatureHumidityChart").getContext("2d");
        new Chart(ctx, {
            type: "line",
            data: {
                labels: timestamps,
                datasets: [
                    {
                        label: "Temperature (°C)",
                        data: temperatureData,
                        borderColor: "#FF6384",
                        backgroundColor: "rgba(255, 99, 132, 0.2)",
                        fill: true,
                        yAxisID: 'y-axis-temp'
                    },
                    {
                        label: "Humidity (%)",
                        data: humidityData,
                        borderColor: "#36A2EB",
                        backgroundColor: "rgba(54, 162, 235, 0.2)",
                        fill: true,
                        yAxisID: 'y-axis-humid'
                    }
                ]
            },
            options: {
                responsive: true,
                scales: {
                    yAxes: [
                        {
                            id: 'y-axis-temp',
                            type: 'linear',
                            position: 'left',
                            scaleLabel: {
                                display: true,
                                labelString: 'Temperature (°C)'
                            }
                        },
                        {
                            id: 'y-axis-humid',
                            type: 'linear',
                            position: 'right',
                            scaleLabel: {
                                display: true,
                                labelString: 'Humidity (%)'
                            }
                        }
                    ]
                },
                title: {
                    display: true,
                    text: 'Temperature and Humidity over Time'
                }
            }
        });
    } catch (error) {
        console.error("Error fetching sensor data:", error);
    }
}

// Load data when the page is loaded
document.addEventListener("DOMContentLoaded", fetchSensorData);
