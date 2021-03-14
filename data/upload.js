
function fileCheck() {
    var inputFile = document.getElementById('uploads');
    if ( inputFile != null) {
        let haveFile = inputFile.fileList;
        if ( haveFile == undefined) {
            console.log("No files selected. Upload disabled.");
            document.getElementById("upButton").disabled = true;
            document.getElementById("upButton").className = 'disabled';        
        }
        console.log("File selection made. Upload enabled.");
        //Enable the submit button.
        document.getElementById("upButton").disabled = false;
        document.getElementById("upButton").className = 'enabled';
    }
}

window.addEventListener("load", startup, false);
function startup() {
    document.getElementById('upButton').disabled = true;
    document.getElementById('upButton').className = 'disabled';
    myFile = document.getElementById('uploads');
//     console.log("Setting up event listener on " + myFile);
    myFile.addEventListener("change", updateAll, false);
    myFile.select();
}
function updateAll(event) { // this will run on input.
    var file = document.getElementById('uploads');
//     console.log("target is: " + file);
    let haveFile = file.value;
//     console.log("got file: " + haveFile);
    if (haveFile) {
//         console.log("target is: " + file);
//         console.log("got file: " + haveFile);
        console.log("File selection made. Upload enabled.");
        // Enable the submit button.
        document.getElementById("upButton").disabled = false;
        document.getElementById("upButton").className = 'enabled';
    } else {
        console.log("No file selection made.");
        console.log("target is: " + file);
        console.log("got file: " + haveFile);
        // Disable the submit button.
        document.getElementById("upButton").disabled = true;
        document.getElementById("upButton").className = 'disabled';
    }
}
