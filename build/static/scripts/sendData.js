function sendData(){
    var formElements = document.getElementById("settings-form").elements;
    
    var data = JSON.stringify({
        "threshold":parseFloat(formElements["threshold"].value), 
        "range":parseFloat(formElements["range"].value)});

    $.post("/setParams",data,
    function(response,status)
    {
        console.log(status);
        if(status == "success")
        {
            document.getElementById("threshold").value = "";
            document.getElementById("range").value = "";
            document.getElementById("success-tag").style.display = "block";
            console.log("lmao");
        }
    });
}