let recv_data = {};

function updateData() 
{
    console.log("updateData()");

    const xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() 
    {
        if (this.readyState == 4 && this.status == 200) 
        {
            console.log("updateData(): got Data:", this.responseText);
            recv_data = JSON.parse(this.responseText);
        }
    };
    xhttp.open("GET", "/getcalib", true);
    xhttp.send();
}

const plan_1_info = document.getElementById("plan-1-info-btn");
plan_1_info.addEventListener("click", (ev) => {
    window.location.href = "information.htm";
    ev.stopPropagation();
});

const plan_2_info = document.getElementById("plan-2-info-btn");
plan_2_info.addEventListener("click", (ev) => {
    window.location.href = "information.htm";
    ev.stopPropagation();
});

const plan_3_info = document.getElementById("plan-3-info-btn");
plan_3_info.addEventListener("click", (ev) => {
    window.location.href = "information.htm";
    ev.stopPropagation();
});

const plan_4_info = document.getElementById("plan-4-info-btn");
plan_4_info.addEventListener("click", (ev) => {
    window.location.href = "information.htm";
    ev.stopPropagation();
});

const plan_5_info = document.getElementById("plan-5-info-btn");
plan_5_info.addEventListener("click", (ev) => {
    window.location.href = "information.htm";
    ev.stopPropagation();
});

const plan_6_info = document.getElementById("plan-6-info-btn");
plan_6_info.addEventListener("click", (ev) => {
    window.location.href = "information.htm";
    ev.stopPropagation();
});

const plan_7_info = document.getElementById("plan-7-info-btn");
plan_7_info.addEventListener("click", (ev) => {
    window.location.href = "information.htm";
    ev.stopPropagation();
});

const plan_8_info = document.getElementById("plan-8-info-btn");
plan_8_info.addEventListener("click", (ev) => {
    window.location.href = "information.htm";
    ev.stopPropagation();
});

const plan_9_info = document.getElementById("plan-9-info-btn");
plan_9_info.addEventListener("click", (ev) => {
    window.location.href = "information.htm";
    ev.stopPropagation();
});

function onPlanSelect(planNumber) {
    var element1 = document.getElementsByClassName("plan-part-b");
    for(var i=0;i<element1.length;i++) {
        element1[i].classList.remove("display-inline-flex");
    }
    switch(planNumber) {
        case 1:
            var element2 = document.getElementById("plan-1-saw-thickness-id");
            element2.innerText = recv_data.saw_0_thickness;
          break;
        case 4:
            var element3 = document.getElementById("plan-4-saw-thickness-id");
            element3.innerText = recv_data.saw_0_thickness;
          break;
        case 5:
            var element4 = document.getElementById("plan-5-saw-thickness-id");
            element4.innerText = recv_data.saw_0_thickness;
          break;
        default:
          break;
    }
    planName = "plan-"+String(planNumber)+"-param-id";
    element1 = document.getElementById(planName);
    element1.classList.add("display-inline-flex");
}

function onStartCut(planNumber) {
    var startCut_data = { };
    var x = 0;
    var y = 0;
    var z = 0;

    switch(planNumber) {
        case 1:
            
            break;
        case 2:
            var element1 = document.getElementById("plan-2-cutlength-value-id");
            x = element1.value;
            break;
        case 3:
            var element1 = document.getElementById("plan-3-cutlength-value-id");
            var element2 = document.getElementById("plan-3-boardlength-value-id");
            x = element1.value;
            y = element2.value;
            break;
        case 4:
            var element3 = document.getElementById("plan-4-incrementalcut-value-id");
            x = element3.value;
            break;
        case 5:
            var element4 = document.getElementById("plan-5-boardlength-value-id");
            var element5 = document.getElementById("plan-5-centralcutlength-value-id");
            var element6 = document.getElementById("plan-5-toothlength-value-id");
            x = element4.value;
            y = element5.value;
            z = element6.value;
            break;
        case 6:
            var element7 = document.getElementById("plan-6-centralcutlength-value-id");
            var element8 = document.getElementById("plan-6-boardlength-value-id");
            x = element7.value;
            y = element8.value;
            break;
        case 7:
            var element9 = document.getElementById("plan-7-centralcutlength-value-id");
            var element10 = document.getElementById("plan-7-boardlength-value-id");
            x = element9.value;
            y = element10.value;
            break;
        case 8:
            var element11 = document.getElementById("plan-8-centralcutlength-value-id");
            var element12 = document.getElementById("plan-8-boardlength-value-id");
            x = element11.value;
            y = element12.value;
            break;
        case 9:
            var element13 = document.getElementById("plan-9-centralcutlength-value-id");
            var element14 = document.getElementById("plan-9-boardlength-value-id");
            x = element13.value;
            y = element14.value;
            break;
        default:
          // code block
    }
    
    startCut_data.plan = planNumber;
    startCut_data.x = x;
    startCut_data.y = y;
    startCut_data.z = z;

    var json = JSON.stringify(startCut_data);
    
    var http = new XMLHttpRequest();            
    var url = '/startcut';

    http.open('POST', url, true);
    http.setRequestHeader("Content-type", "application/json; charset=utf-8");

    http.onreadystatechange = function() 
    {
        if(http.readyState == 4 && http.status == 200) 
        {
            console.log(this.responseText);
            window.location.href = 'cut.htm';
        }
    }
    http.send(json);
}
