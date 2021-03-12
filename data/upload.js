
function fileCheck() {
    haveFiles = document.querySelector("#uploads").FileList;
    if ( haveFiles == null ) {
        document.getElementById("upButton").disabled = false;
    } else {
        //Otherwise, disable the submit button.
        document.getElementById("upButton").disabled = true;
    }
}
