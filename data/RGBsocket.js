/**
 * Copyright 2020-2024 Winford (Uncle Grumpy) <winford@object.stream>
 * SPDX-License-Identifier: MIT
 */

var moonColor; // name of our color chooser
var webrgb; // current color in #rrggbb format
var savedColor; // for watching if save preference should be enabled.
var rainbowEnable = false;
var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);

window.onbeforeunload = function() {
    connection.onclose = function () {}; // disable onclose handler first
    connection.close();
}

window.addEventListener("load", startup, false);

function startup() {
    document.getElementById('save').disabled = true;
    document.getElementById('save').className = 'disabled';
    moonColor = document.querySelector("#moonColor");
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
function updateAll(event){ // runs after selection confirmed.
    var input = document.querySelector("#moonColor");
    if (input) {
        webrgb = event.target.value;
        sendRGB();
        input.style.color = webrgb;
    }
}
connection.onopen = function () {
    connection.send('Connect ' + new Date());
    console.log('WebSocket connection established.');
}
connection.onerror = function (error) {
    console.log('WebSocket Error ', error);
}
connection.onmessage = function (e) {
    if (e.data[0] == "#") {
        // console.log("Server sent " + e.data);
        webrgb = e.data;
        // console.log("savedColor is " + savedColor);
        if ( savedColor == undefined ) {
            if ( rainbowEnable === false ) {
                savedColor = webrgb + "-";
                // console.log("savedColor is set to " + savedColor + " webrgb is " + webrgb);
            } else {
                savedColor =  webrgb + "+";
                // console.log("savedColor is set to " + savedColor + " webrgb is " + webrgb);
            }
        }
        document.getElementById('moon').style.backgroundColor = webrgb;
        document.querySelector("#moonColor").value = webrgb;
        if ( rainbowEnable === false ) {
            if (savedColor != webrgb + "-") {
                document.getElementById('save').disabled = false;
                document.getElementById('save').className = 'enabled';
            } else {
                document.getElementById('save').disabled = true;
                document.getElementById('save').className = 'disabled';
            }
        } else {
            if ( savedColor[7] != "+") {
                document.getElementById('save').disabled = false;
                document.getElementById('save').className = 'enabled';
            } else {
                document.getElementById('save').disabled = true;
                document.getElementById('save').className = 'disabled';
            }
        }
        // console.log("Server set color to: " + webrgb);
    } else if (e.data[0] == "R") {
        rainbowEnable = true;
        // console.log("Server sent: " + e.data + " -- activate rainbow.");
        // console.log("savedColor is set to " + savedColor + " webrgb is " + webrgb);
        if (savedColor[7] != "+" ) {
            document.getElementById('save').disabled = false;
            document.getElementById('save').className = 'enabled';
        } else {
            document.getElementById('save').disabled = true;
            document.getElementById('save').className = 'disabled';
        }
        document.getElementById('moonColor').className = 'disabled';
        document.getElementById('moonColor').disabled = true;
        document.getElementById('rainbow').className = 'enabled';
    } else if (e.data[0] == "N") {
        rainbowEnable = false;
        // console.log("Server sent: " + e.data + " -- deactivate rainbow.");
        // console.log("savedColor is set to " + savedColor + " webrgb is " + webrgb);
        if (savedColor != webrgb + "-") {
            document.getElementById('save').disabled = false;
            document.getElementById('save').className = 'enabled';
        } else {
            document.getElementById('save').disabled = true;
            document.getElementById('save').className = 'disabled';
        }
        document.getElementById('moon').style.backgroundColor = webrgb;
        document.getElementById('moonColor').className = 'enabled';
        document.getElementById('moonColor').disabled = false;
        document.getElementById('rainbow').className = 'disabled';
    } else if (e.data[0] == "S") {
        if (e.data[1] == "y") {
            if ( rainbowEnable === false ) {
                savedColor = webrgb + "-";
            } else {
                var ACTIVE = "+";
                var setNew = savedColor.substring(0,  7) + ACTIVE + savedColor.substring(8);
                savedColor = setNew;
            }
            console.log("Success! savedColor is set to " + savedColor + " webrgb is " + webrgb);
            alert ("Color settings were saved. The next time the Moonlamp is turned on these settings will be used.");
            connection.send("C");
        } else {
            console.log("Failed! savedColor is set to " + savedColor + " webrgb is " + webrgb);
            alert ("Failed to update settings! Please report this!");
        }
    } else {
        console.log("Unknown data received: " + e.data);
    }
}

connection.onclose = function() {
    console.log("WebSocket connection closed.");
}

function sendRGB() {
    connection.send(webrgb);
}

function rainbowEffect() {
    rainbowEnable = ! rainbowEnable;
    if(rainbowEnable){
        connection.send("R");
    } else {
        connection.send("N");
    }
}

function saveColor() {
    connection.send("S" + webrgb);
    console.log("Requesting save, sent: S" + webrgb + " to server.");
}
