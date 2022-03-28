function refresh(){
    setTimeout(refresh,5000);
    $.get("/getParams", function(data,status){
        $("#temperature").html(data["temp"]+"&#8451");
        document.getElementById("maxTemp").placeholder = data["maxTemp"];
        document.getElementById("minTemp").placeholder = data["minTemp"];
        document.getElementById("state").innerHTML = data["state"];

        if(data["state"] == "ON")
        {
            document.getElementById("state").style = "color: green";
        }
        else
        {
            document.getElementById("state").style = "color: red";
        }
    });
}

refresh();