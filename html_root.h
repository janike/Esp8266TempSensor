const char htmlRoot[] PROGMEM = R"=====(
<!DOCTYPE html>
<html class="" lang="en">
<head>
<meta http-equiv="content-type" content="text/html; charset=UTF-8">
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>ESP8266 TempSensor</title>
<script>
  var tt;
  function rr(p=""){
      console.log("rr method");
      var xmlHttp = new XMLHttpRequest();
        xmlHttp.onreadystatechange = function() { 
            if (xmlHttp.readyState == 4 && xmlHttp.status == 200) {
              clearTimeout(tt);
              var el;
              var values = JSON.parse(xmlHttp.responseText);
              el = document.getElementById('data-voltage');
              if(el) el.innerHTML = values['voltage'] + ' V';;
              el = document.getElementById('data-temperature');
              if(el) el.innerHTML = parseFloat(values['temperature']).toFixed(2) + '&#8451';
              el = document.getElementById('data-humidity');
              if(el) el.innerHTML = parseFloat(values['humidity']).toFixed(0) + '%';
              tt=setTimeout(rr, 5000);
            }
        }
        xmlHttp.open("GET", '/check?'+p, true);
        xmlHttp.send(null);
        tt=setTimeout(rr,10000);
  }

  (function() {
    rr();
  })();

</script>
  
  <style>
    div,fieldset,input,select{padding:5px;font-size:1em;}
    fieldset{background:#4f4f4f;}p{margin:0.5em 0;}
    input{width:100%;box-sizing:border-box;-webkit-box-sizing:border-box;-moz-box-sizing:border-box;background:#dddddd;color:#000000;}
    input[type=checkbox],input[type=radio]{width:1em;margin-right:6px;vertical-align:-1px;}
    input[type=range]{width:99%;}
    select{width:100%;background:#dddddd;color:#000000;}
    textarea{resize:vertical;width:98%;height:318px;padding:5px;overflow:auto;background:#1f1f1f;color:#65c115;}
    body{text-align:center;font-family:verdana,sans-serif;background:#252525;}td{padding:0px;}
    button{border:0;border-radius:0.3rem;background:#1fa3ec;color:#faffff;line-height:2.4rem;font-size:1.2rem;width:100%;-webkit-transition-duration:0.4s;transition-duration:0.4s;cursor:pointer;}
    button:hover{background:#0e70a4;}.bred{background:#d43535;}.bred:hover{background:#931f1f;}.bgrn{background:#47c266;}.bgrn:hover{background:#5aaf6f;}a{color:#1fa3ec;text-decoration:none;}.p{float:left;text-align:left;}.q{float:right;text-align:right;}.r{border-radius:0.3em;padding:2px;margin:6px 2px;}.hf{display:none;}
</style>
</head>
<body>
  <div style="text-align:left;display:inline-block;color:#eaeaea;min-width:340px;"><div style="text-align:center;color:#eaeaea;">
  <noscript>To use This, enable javascript<br></noscript>
  <h2>ESP8266 TempSensor</h2></div><div style="padding:0;" id="l1" name="l1">
    <table style="width:100%"></table><table style="width:100%">
      <tbody>
        <tr><td id="data-temperature" style="width:100%;text-align:center;font-weight:normal;font-size:62px">--- &#8451</td></tr>
        <!-- UNCOMMENT NEXT LINE IF SENSOR is DHT TYPE -->
        <!--<tr><td id="data-humidity" style="width:100%;text-align:center;font-weight:normal;font-size:62px">--- %</td></tr
        <tr><td id="data-voltage" style="width:100%;text-align:center;font-weight:normal;font-size:31px">0 V</td></tr>-->
        <tr></tr>
      </tbody>
    </table>
  </div>
  <br><br>
  
  <form id="but3" style="display: block;" action="/config" method="get">
    <button name="">Configuration</button>
  </form><p></p>
  <div style="text-align:right;font-size:11px;"><hr><a href="https://bit.ly/tasmota" target="_blank" style="color:#aaa;">Tasmota GUI by Theo Arends</a></div>

</body>
</html>
)=====";
