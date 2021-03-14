
function fileCheck() {
    var inputFile = document.getElementById('uploads');
    if ( inputFile != null) {
        let haveFile = inputFile.fileList;
        if ( haveFile == undefined) {
//             console.log("No files selected. Upload disabled.");
            document.getElementById("upButton").disabled = true;
            document.getElementById("upButton").className = 'disabled';        
        }
        console.log("File selection made. Upload enabled.");
        // Enable the submit button.
        document.getElementById("upButton").disabled = false;
        document.getElementById("upButton").className = 'enabled';
    }
}
 
//         if ( haveFile != null) {
//             console.log("File selection made. Upload enabled.");
//             //Enable the submit button.
//             document.getElementById("upButton").disabled = false;
//             document.getElementById("upButton").className = 'enabled';
//         } else {
//             console.log("No files selected. Upload disabled.");
//             document.getElementById("upButton").disabled = true;
//             document.getElementById("upButton").className = 'disabled';
//         }
//     }
// }
