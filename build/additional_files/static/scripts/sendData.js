function sendData(){
    var formElements = document.getElementById("settings-form").elements;
    
    var data = JSON.stringify({
        "minTemp":parseFloat(formElements["minTemp"].value), 
        "maxTemp":parseFloat(formElements["maxTemp"].value)});

    $.post("/setParams",data,
    function(response,status)
    {
        console.log(status);
        if(status == "success")
        {
            document.getElementById("minTemp").value = "";
            document.getElementById("maxTemp").value = "";
            document.getElementById("success-tag").style.display = "block";
        }
    });
}