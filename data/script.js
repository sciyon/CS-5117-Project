var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
// Init web socket when the page loads
window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
}

function getReadings(){
    websocket.send("getReadings");
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

// When websocket is established, call the getReadings() function
function onOpen(event) {
    console.log('Connection opened');
    getReadings();
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function updateFFTChart(fftData) {
  const { label, frequencies, magnitudes } = fftData;

  const fftChart = Chart.getChart("fftChart");

  if (label === 'temperature') {
    fftChart.data.datasets[0].data = magnitudes;
  } else if (label === 'humidity') {
    fftChart.data.datasets[1].data = magnitudes;
  } else if (label === 'gas') {
    fftChart.data.datasets[2].data = magnitudes;
  }

  fftChart.data.labels = frequencies; // Shared labels
  fftChart.update();
}

// Function that receives the message from the ESP32 with the readings
function onMessage(event) {
  console.log(event.data);
  const data = JSON.parse(event.data);

  // Update FFT chart if FFT data is present
  if (data.frequencies && data.magnitudes) {
      updateFFTChart(data);
  }

  // Update live sensor readings if sensor data is present
  if (data.temperature && data.humidity && data.gas) {
      document.getElementById("temperature").innerHTML = data["temperature"];
      document.getElementById("humidity").innerHTML = data["humidity"];
      document.getElementById("gas").innerHTML = data["gas"];

      const timeLabel = new Date().toLocaleTimeString();
      chart.data.labels.push(timeLabel);
      chart.data.datasets[0].data.push(parseFloat(data["temperature"]));
      chart.data.datasets[1].data.push(parseFloat(data["humidity"]));
      chart.data.datasets[2].data.push(parseFloat(data["gas"]));
      chart.update();
  }
}