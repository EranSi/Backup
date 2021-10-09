let recv_data = {};
let paramsRecvFlag = 0;

function updateData() {
    console.log("updateData()");

    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            console.log("updateData(): got Data:", this.responseText);

            recv_data = JSON.parse(this.responseText);

            if (recv_data.max_num_saws != 2) {
                alert('JS does not support num_saws != 2');
            }
            else {
                paramsRecvFlag = 1;
                onLoadSettings();
            }
        }
    };
    xhttp.open("GET", "/getcalib", true);
    xhttp.send();
}

function onLoadSettings() {
    var element1 = document.getElementById("saw-pressure-range");
    var element2 = document.getElementById("pressure-val-id");
    if (recv_data.installed_saw == "0") {
        regularSawUpdateView();
        element1.value = recv_data.saw_0_pressure;
        element2.innerText = recv_data.saw_0_pressure;
    }
    else if (recv_data.installed_saw == "1") {
        dovetailBitUpdateView();
        element1.value = recv_data.saw_1_pressure;
        element2.innerText = recv_data.saw_1_pressure;
    }
    else {
        alert('Unsupported saw number');
    }
    onSaveSettings();
}

function sawSelect() {
    var element = document.getElementById("saw-select-content-id");
    element.classList.add("display-inline-flex-z");
}

function regularSawUpdateView() {
    var element1 = document.getElementById("saw-select-current-id");
    element1.innerText = "Regular Saw 1";
    var element2 = document.getElementById("saw-img-id");
    element2.innerHTML = "<img src=\"img/imgRegularSaw1.svg\">";
    var element3 = document.getElementById("saw-thickness-id");
    element3.children[0].value = recv_data.saw_0_thickness;
    element3.classList.add("display-inline-flex");
    var element4 = document.getElementById("d-thickness-id");
    element4.classList.remove("display-inline-flex");
    var element5 = document.getElementById("h-thickness-id");
    element5.classList.remove("display-inline-flex");
    var element6 = document.getElementById("D-thickness-id");
    element6.classList.remove("display-inline-flex");
    var element7 = document.getElementById("saw-select-content-id");
    element7.classList.remove("display-inline-flex-z");
    if (recv_data.saw_0_units == "0") {
        mmUnitsSelect();
    }
    else {
        inchUnitsSelect();
    }
}

function regularSawSelect() {
    recv_data.installed_saw = "0";
    regularSawUpdateView();
    var element1 = document.getElementById("saw-pressure-range");
    var element2 = document.getElementById("pressure-val-id");
    element1.value = recv_data.saw_0_pressure;
    element2.innerText = recv_data.saw_0_pressure;
    onChangeSettings();
}

function dovetailBitUpdateView() {
    var element1 = document.getElementById("saw-select-current-id");
    element1.innerText = "Dovetail Bit 2";
    var element2 = document.getElementById("saw-img-id");
    element2.innerHTML = "<img src=\"img/imgDovetailBit1.svg\">";
    var element3 = document.getElementById("saw-thickness-id");
    element3.classList.remove("display-inline-flex");
    var element4 = document.getElementById("d-thickness-id");
    element4.children[0].value = recv_data.saw_1_thickness_d;
    element4.classList.add("display-inline-flex");
    var element5 = document.getElementById("h-thickness-id");
    element5.children[0].value = recv_data.saw_1_thickness_h;
    element5.classList.add("display-inline-flex");
    var element6 = document.getElementById("D-thickness-id");
    element6.children[0].value = recv_data.saw_1_thickness_D;
    element6.classList.add("display-inline-flex");
    var element7 = document.getElementById("saw-select-content-id");
    element7.classList.remove("display-inline-flex-z");
    if (recv_data.saw_1_units == "0") {
        mmUnitsSelect();
    }
    else {
        inchUnitsSelect();
    }
}

function dovetailBitSelect() {
    recv_data.installed_saw = "1";
    dovetailBitUpdateView();
    var element1 = document.getElementById("saw-pressure-range");
    var element2 = document.getElementById("pressure-val-id");
    element1.value = recv_data.saw_1_pressure;
    element2.innerText = recv_data.saw_1_pressure;
    onChangeSettings();
}

function unitsSelect() {
    var element = document.getElementById("units-select-content-id");
    element.classList.add("display-inline-flex-z");
}

function mmUnitsSelect() {
    if (recv_data.installed_saw == "0") {
        recv_data.saw_0_units = "0";
    }
    else {
        recv_data.saw_1_units = "0";
    }
    var element1 = document.getElementById("units-select-current-id");
    element1.innerText = "MM";
    var element2 = document.getElementById("units-select-content-id");
    element2.classList.remove("display-inline-flex-z");
    onChangeSettings();
}

function inchUnitsSelect() {
    if (recv_data.installed_saw == "0") {
        recv_data.saw_0_units = "1";
    }
    else {
        recv_data.saw_1_units = "1";
    }
    var element1 = document.getElementById("units-select-current-id");
    element1.innerText = "Inch";
    var element2 = document.getElementById("units-select-content-id");
    element2.classList.remove("display-inline-flex-z");
    onChangeSettings();
}

function changeSawPressure(value) {
    const element = document.getElementById("pressure-val-id");
    element.innerHTML = value;
    if (recv_data.installed_saw == "0") {
        recv_data.saw_0_pressure = value;
    }
    else if (recv_data.installed_saw == "1") {
        recv_data.saw_1_pressure = value;
    }
    else {
        alert('Unsupported saw number');
    }
    onChangeSettings();
}

function setHomePos() {
    var homeParams = {};
    var json = JSON.stringify(homeParams);

    var http = new XMLHttpRequest();
    var url = '/sethome';

    http.open('POST', url, true);
    http.setRequestHeader("Content-type", "application/json; charset=utf-8");

    http.onreadystatechange = function () {
        if (http.readyState == 4 && http.status == 200) {
            console.log(this.responseText);
        }
    }

    http.send(json);
}

function directionSend(direction) {
    var directionParams = {};
    directionParams.direction = direction;
    var json = JSON.stringify(directionParams);

    var http = new XMLHttpRequest();
    var url = '/direction';

    http.open('POST', url, true);
    http.setRequestHeader("Content-type", "application/json; charset=utf-8");

    http.onreadystatechange = function () {
        if (http.readyState == 4 && http.status == 200) {
            console.log(this.responseText);
        }
    }

    http.send(json);
}

function left() {
    directionSend(1);
}

function right() {
    directionSend(-1);
}

function speedSelect(speed) {
    var speedParams = {};
    speedParams.speed = speed;
    var json = JSON.stringify(speedParams);

    var http = new XMLHttpRequest();
    var url = '/speed';

    http.open('POST', url, true);
    http.setRequestHeader("Content-type", "application/json; charset=utf-8");

    http.onreadystatechange = function () {
        if (http.readyState == 4 && http.status == 200) {
            console.log(this.responseText);
        }
    }
    http.send(json);
}

function x1() {
    var x1_btn = document.getElementById("x1-id");
    var x10_btn = document.getElementById("x10-id");
    var x100_btn = document.getElementById("x100-id");

    x1_btn.classList.add("motor-speed-btn-color-selected");
    x10_btn.classList.remove("motor-speed-btn-color-selected");
    x100_btn.classList.remove("motor-speed-btn-color-selected");

    speedSelect(1);
}

function x10() {
    var x1_btn = document.getElementById("x1-id");
    var x10_btn = document.getElementById("x10-id");
    var x100_btn = document.getElementById("x100-id");

    x1_btn.classList.remove("motor-speed-btn-color-selected");
    x10_btn.classList.add("motor-speed-btn-color-selected");
    x100_btn.classList.remove("motor-speed-btn-color-selected");

    speedSelect(10);
}

function x100() {
    var x1_btn = document.getElementById("x1-id");
    var x10_btn = document.getElementById("x10-id");
    var x100_btn = document.getElementById("x100-id");

    x1_btn.classList.remove("motor-speed-btn-color-selected");
    x10_btn.classList.remove("motor-speed-btn-color-selected");
    x100_btn.classList.add("motor-speed-btn-color-selected");

    speedSelect(100);
}

function saveLocal() {
    if (recv_data.installed_saw == "0") {
        var element1 = document.getElementById("saw-thickness-id");
        recv_data.saw_0_thickness = element1.children[0].value;
        var element2 = document.getElementById("units-select-current-id");
        if (element2.innerText == "MM") {
            recv_data.saw_0_units = "0";
        }
        else {
            recv_data.saw_0_units = "1";
        }
    }
    else if (recv_data.installed_saw == "1") {
        var element1 = document.getElementById("d-thickness-id");
        recv_data.saw_1_thickness_d = element1.children[0].value;
        var element2 = document.getElementById("h-thickness-id");
        recv_data.saw_1_thickness_h = element2.children[0].value;
        var element3 = document.getElementById("D-thickness-id");
        recv_data.saw_1_thickness_D = element3.children[0].value;
        var element4 = document.getElementById("units-select-current-id");
        if (element4.innerText == "MM") {
            recv_data.saw_1_units = "0";
        }
        else {
            recv_data.saw_1_units = "1";
        }
    }
    else {
        alert('Unsupported saw number'); //TODO: Move this alert
    }
}

function saveData() {
    if (paramsRecvFlag == 0) {
        alert("Please wait until the page is fully loaded."); //TODO: Move this alert
    }
    saveLocal();

    var json = JSON.stringify(recv_data);

    var http = new XMLHttpRequest();
    var url = '/savecalib';

    http.open('POST', url, true);
    http.setRequestHeader("Content-type", "application/json; charset=utf-8");

    http.onreadystatechange = function () {
        if (http.readyState == 4 && http.status == 200) {
            console.log(this.responseText);
            const element = document.getElementById("img-tick-square-id");
            element.src = "img/imgTickSquareV.svg";
        }
    }

    http.send(json);

}



function onChangeSettings() {
    var element = document.getElementById("img-tick-square-id");
    element.src = "img/imgTickSquare.svg"
}

function onSaveSettings() {
    var element = document.getElementById("img-tick-square-id");
    element.src = "img/imgTickSquareV.svg"
}