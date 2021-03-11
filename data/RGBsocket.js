
var moonColor;   //name of our color chooser
var webrgb;         // last user chosen color in #rrggbb format
var defaultColor // value for the chooser

window.addEventListener("load", startup, false);

function setup() {
    if ( document.readyState == "complete" ) {
        updateColor();
    }
}
function startup() {
    moonColor = document.querySelector("#moonColor");
    if ( moonColor == null ) {
        updateColor();
        moonColor.value = webrgb;
    }
    moonColor.addEventListener("input", updateFirst, false);
    moonColor.addEventListener("change", updateAll, false);
    moonColor.select();
}
function updateFirst(event) { // this will run on input.
    var input = document.querySelector("#moonColor");
    if (input) {
        webrgb = event.target.value;
        sendRGB();
    }
}
function updateAll(event) { // runs after selection confirmed.
    var input = document.querySelector("#moonColor");
    if (input) {
        webrgb = event.target.value;
        sendRGB();
        input.style.color = webrgb;        
    }
}

var rainbowEnable;
var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);

connection.onopen = function () {
    connection.send('Connect ' + new Date());
    console.log('WebSocket connection established.');
}
connection.onerror = function (error) {
    console.log('WebSocket Error ', error);
}
connection.onmessage = function (e) {  
    if (e.data[0] == "#") {
        defaultColor = e.data;
        webrgb = e.data;
        document.getElementById('moon').style.backgroundColor = webrgb;
        document.querySelector("#moonColor").value = webrgb;
        console.log("Server set color to: " + webrgb);
    } else if (e.data[0] == "R") {
        rainbowEnable = true;
        console.log("Server sent: " + e.data + " -- activate rainbow.");
        document.getElementById('moonColor').className = 'disabled';
        document.getElementById('moonColor').disabled = true;
        document.getElementById('rainbow').className = 'enabled';
    } else if (e.data[0] == "N") {
        rainbowEnable = false;
        console.log("Server sent: " + e.data + " -- deactivate rainbow.");
        document.getElementById('moon').style.backgroundColor = webrgb;
        document.getElementById('moonColor').className = 'enabled';
        document.getElementById('moonColor').disabled = false;
        document.getElementById('rainbow').className = 'disabled';
    } else if (e.data[0] == "S") {
        if (e.data[1] == "y") {
            alert ("Color settings were saved. The next time the Moonlamp is turned on these settings will be used.");
//            document.write ("Color settings were saved. The next time the Moonlamp is turned on these settings will be used.");
        } else {
            alert ("Failed to update settings! Please report this!");
//            document.write ("Failed to update settings! Please report this!");
        } 
    } else {
        console.log("Unknown data received: " + e.data);
    }
}
connection.onclose = function(){
    console.log('WebSocket connection closed.');
}
function sendRGB(){
    connection.send(webrgb);
}
function updateColor(){
    console.log("Requesting color from server.");
    connection.send("C");
    while (webrgb[0] != "#" ) {
        console.log("still waiting for color...");
        wait(500);
    }
}
function rainbowEffect(){
    rainbowEnable = ! rainbowEnable;
    if(rainbowEnable){
        connection.send("R");
    } else {
        connection.send("N");
    }  
}
function saveColor(){
    connection.send("S" + webrgb);
    console.log("Requesting save, sent: S" + webrgb + " to server.");
}
