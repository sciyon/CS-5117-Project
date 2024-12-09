var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

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

function onMessage(event) {
  var myObj = JSON.parse(event.data);
  var keys = Object.keys(myObj);
  console.log(event.myObj);

  for (var i = 0; i < keys.length; i++){
      var key = keys[i];
      document.getElementById(key).innerHTML = myObj[key];
  }
  // updateChart(myObj.temperature, myObj.humidity, myObj.gas);

}