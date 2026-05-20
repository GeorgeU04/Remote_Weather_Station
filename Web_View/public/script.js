const temperatureLabel = document.getElementById("temperatureLabel");
const humidityLabel = document.getElementById("humidityLabel");
const pressureLabel = document.getElementById("pressureLabel");

const socket = new WebSocket("ws://localhost:3000");

let temperature = 0;
let humidity = 0;
let pressure = 0;
socket.addEventListener("message", (event) => {
  const line = event.data.trim();
  if (line.startsWith("Temperature:")) {
    temperature = parseFloat(line.split(":")[1].trim()) / 100;
    temperatureLabel.textContent = temperature;
  }
  else if (line.startsWith("Humidity:")) {
    humidity = (parseFloat(line.split(":")[1].trim()) / 1024).toFixed(2);
    humidityLabel.textContent = humidity;
  }
  else if (line.startsWith("Pressure:")) {
    pressure = parseFloat((line.split(":")[1].trim()) / 256).toFixed(2);
    pressureLabel.textContent = pressure;
  }
});

