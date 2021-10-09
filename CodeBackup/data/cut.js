var cutCounter = 0;
var recv_data = { };
//var installed_saw = 0;
var sawBusyFlag = 0;

function updateData(){
    resetCutCounter();
    boardtype_a();
    
    console.log("updateData()");
    
    var xhttp = new XMLHttpRequest();
    
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            console.log("updateData(): got Data:", this.responseText);

            recv_data = JSON.parse(this.responseText);

            if (recv_data.max_num_saws != 2) {
                alert('JS does not support num_saws != 2');
            }
            else {
                installed_saw = recv_data.installed_saw;
            }
        }
    };
    xhttp.open("GET", "/getcalib", true);
    xhttp.send();
}

function saveData() {

    var json = JSON.stringify(recv_data);
    var http = new XMLHttpRequest();
    var url = '/savecalib';

    http.open('POST', url, true);
    http.setRequestHeader("Content-type", "application/json; charset=utf-8");
    http.onreadystatechange = function() {
        if(http.readyState == 4 && http.status == 200) {
            alert(http.responseText);
        }
    }
    http.send(json);
}

function resetCutCounter() {
    cutCounter=0;
    document.getElementById("counter-id").value = cutCounter;
}

function forwardCutting() {
    if(sawBusyFlag == 1) {
        // if saw is busy, we just bail
        return;
    }
    sawBusy();
    cutCounter++;
    document.getElementById("counter-id").value = cutCounter;

    var forwardParams= {};
    var json = JSON.stringify(forwardParams);
    var http = new XMLHttpRequest();   
    var url = '/forward';

    http.open('POST', url, true);
    http.setRequestHeader("Content-type", "application/json; charset=utf-8");
    http.onreadystatechange = function() {
        if(http.readyState == 4 && http.status == 200) {
            var responseData = JSON.parse(this.responseText);

            if (responseData["is_saw_busy"]==true) {
                sawBusy();                              
            }
            else {
                // saw no longer busy, hide the box overlay
                sawIdle();
            }
        }
    }
    http.send(json);
}

function reverseCutting() {
    if(sawBusyFlag == 1) {
        // if saw is busy, we just bail
        return;
    }
    cutCounter--;
    document.getElementById("counter-id").value = cutCounter;
    sawBusy();    

    var reverseParams= {};
    var json = JSON.stringify(reverseParams);
    var http = new XMLHttpRequest();            
    var url = '/reverse';

    http.open('POST', url, true);
    http.setRequestHeader("Content-type", "application/json; charset=utf-8");
    http.onreadystatechange = function() {
        if(http.readyState == 4 && http.status == 200) {
            var responseData = JSON.parse(this.responseText);

            if (responseData["is_saw_busy"]==true) {
                sawBusy();                              
            }
            else {
                // saw no longer busy, hide the box overlay
                sawIdle();
            }
        }
    }
    if(cutCounter <= 0)
    {
        resetCutCounter();
    }
    http.send(json);
}

function homePosition() {
    if(sawBusyFlag == 1) {
        // if saw is busy, we just bail
        return;
    }
    resetCutCounter();
    sawBusy();
    
    var homeParams= {};
    var json = JSON.stringify(homeParams);
    var http = new XMLHttpRequest();            
    var url = '/home';

    http.open('POST', url, true);
    http.setRequestHeader("Content-type", "application/json; charset=utf-8");
    http.onreadystatechange = function() {
        if(http.readyState == 4 && http.status == 200) {
            var responseData = JSON.parse(this.responseText);

            if (responseData["is_saw_busy"]==true) {
                sawBusy();                              
            }
            else {
                // saw no longer busy, hide the box overlay
                sawIdle();
            }
        }
    }
    http.send(json);
}

function boardtype(btype) {
    if(sawBusyFlag == 1) {
        // if saw is busy, we just bail
        return;
    }

    var boardtypeParams= {};

    boardtypeParams.boardType = btype;

    var json = JSON.stringify(boardtypeParams);
    var http = new XMLHttpRequest();            
    var url = '/boardtype';

    http.open('POST', url, true);
    http.setRequestHeader("Content-type", "application/json; charset=utf-8");
    http.onreadystatechange = function() {
        if(http.readyState == 4 && http.status == 200) {
            console.log(http.responseText);
        }
    }
    http.send(json);
}

function boardtype_a() {
    if(sawBusyFlag == 1) {
        // if saw is busy, we just bail
        return;
    }
    resetCutCounter(); 
    var boardtype_a_btn = document.getElementById("board-a-id");
    var boardtype_b_btn = document.getElementById("board-b-id");
    
    boardtype_a_btn.classList.add("board-btn-color-selected");
    boardtype_b_btn.classList.remove("board-btn-color-selected");
    
    var element = document.getElementById("img-board-active-container-id");
    element.innerHTML = "<img src=\"img/plan3symmetricalA.png\" height=\"280px\">";
    boardtype("0");
}

function boardtype_b() {
    if(sawBusyFlag == 1) {
        // if saw is busy, we just bail
        return;
    }
    resetCutCounter(); 

    var boardtype_a_btn = document.getElementById("board-a-id");
    var boardtype_b_btn = document.getElementById("board-b-id");
    
    boardtype_b_btn.classList.add("board-btn-color-selected");
    boardtype_a_btn.classList.remove("board-btn-color-selected");

    var element = document.getElementById("img-board-active-container-id");
    element.innerHTML = "<img src=\"img/plan3symmetricalB.png\" height=\"280px\">";
    boardtype("1");
}

function sawBusy() {
    sawBusyFlag = 1;

    var sawbusydiv = document.getElementById("saw_busy_box");
    
    sawbusydiv.style.visibility='visible';         

    // disable the combo box
    // var obj =  document.getElementById("installed_saw");
    // obj.disabled = true;                 

    // saw is still busy, schedule another check in 2 seconds
    //var myVar = setTimeout(checkSawBusy, 500);
}

function sawIdle() {
    sawBusyFlag = 0;

    // enable combo box
    // var obj =  document.getElementById("installed_saw");
    // obj.disabled = false;

    var sawbusydiv = document.getElementById("saw_busy_box");
    sawbusydiv.style.visibility='hidden';            
}

function checkSawBusy() {
    var sawbusyParams= {};
    var json = JSON.stringify(sawbusyParams);

    var http = new XMLHttpRequest();            
    var url = '/checksawbusy';

    http.open('POST', url, true);
    http.setRequestHeader("Content-type", "application/json; charset=utf-8");
    http.onreadystatechange = function() {
        if(http.readyState == 4 && http.status == 200) {
            var responseData = JSON.parse(this.responseText);

            if (responseData["is_saw_busy"]==true) {
                sawBusy();                              
            }
            else {
                // saw no longer busy, hide the box overlay
                sawIdle();
            }
        }
    }
    http.send(json);
}