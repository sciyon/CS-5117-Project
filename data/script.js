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

// Function that receives the message from the ESP32 with the readings
function onMessage(event) {
    console.log(event.data);
    var myObj = JSON.parse(event.data);

    // Update the HTML elements with the received values
    document.getElementById("temperature").innerHTML = myObj["temperature"];
    document.getElementById("humidity").innerHTML = myObj["humidity"];
    document.getElementById("gas").innerHTML = myObj["gas"];

    // Add the new data to the chart
    const timeLabel = new Date().toLocaleTimeString(); // Current time as label
    chart.data.labels.push(timeLabel);

    // Add temperature, humidity, and gas data to the datasets
    chart.data.datasets[0].data.push(parseFloat(myObj["temperature"]));
    chart.data.datasets[1].data.push(parseFloat(myObj["humidity"]));
    chart.data.datasets[2].data.push(parseFloat(myObj["gas"]));

    // Update the chart
    chart.update();
}