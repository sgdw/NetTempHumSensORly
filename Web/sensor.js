$(document).ready(function() {
    updateSensorData("sensor1.log", "chart1");
    updateSensorData("sensor2.log", "chart2");
});

function updateSensorData(filename, elId) {
    $.ajax({
        type: "GET",
        url: filename,
        dataType: "text",
        success: function(data) {
            var data = processData(data);
            DrawSensorGraph(elId, data);
        }
     });
}

function processData(allText) {
    var record_num = 5;  // or however many elements there are in each row
    var records = allText.split(/\r\n|\n/);
    var dataTemperature = [];
    var dataHumidity = [];

    for(var i = Math.max(0, records.length - 288); i < records.length; i++) {
        var fields = records[i].split(',');
        if(fields.length > 0) {
            fields[0] = dateFromXmldateTime(fields[0]);

            for(var f = 1; f < fields.length; f++) {
                if(fields[f] == "H") dataHumidity.push(   {x: fields[0], y: parseFloat(fields[f+1])})
                if(fields[f] == "T") dataTemperature.push({x: fields[0], y: parseFloat(fields[f+1])})
            }
        }
    }

    var data = {temperature: dataTemperature, humidity: dataHumidity};
    console.log(data);
    return data;
}

function dateFromXmldateTime(s) {
    var parts = s.split(/[-T:]/g);
    var d = new Date(parts[0], parts[1]-1, parts[2]);
    d.setHours(parts[3], parts[4], parts[5]);
    return d;
}

function DrawSensorGraph(elId, data) {

    nv.addGraph(function() {
    var chart = nv.models.lineWithFocusChart()
                .margin({left: 100})  //Adjust chart margins to give the x-axis some breathing room.
                .useInteractiveGuideline(true)  //We want nice looking tooltips and a guideline!
                // .transitionDuration(350)  //how fast do you want the lines to transition?
                .showLegend(true)       //Show the legend, allowing users to turn on/off line series.
                .showYAxis(true)        //Show the y-axis
                .showXAxis(true)        //Show the x-axis
    ;

    chart.xAxis
        // .axisLabel('Zeit')
        .tickFormat(function(d) {
            return d3.time.format('%d.%m. %H:%M')(new Date(d)); // %d.%m.%Y
        });
    chart.xScale(d3.time.scale());

    chart.x2Axis.tickFormat(function(d) {
        return d3.time.format('%d.%m.')(new Date(d)); // %d.%m.%Y
    });

    chart.yAxis     //Chart y-axis settings
        .axisLabel('Temperatur C / Feuchtigkeit %')
        .tickFormat(d3.format('.01f'));

    var funcDemoData = function(factor) {
        var data = [];
        for(var i = 0; i < 30; i++) {
            data.push({x: new Date(2016, 1, i, 12, 10, 0), y: i*factor});
        }
        return data;
    };

    if(data == undefined) {
        data = {temperature: funcDemoData(1), humidity: funcDemoData(0.5)};
    }

    chartData =
    [
        {
            values: data.temperature,
            key: 'Temperatur',
            color: 'rgba(110,110,110,1)',
            area: true
        },
        {
            values: data.humidity,
            key: 'Feuchtigkeit',
            color: 'rgba(151,187,205,1)',
            area: true
        }
    ];

    d3.select('#'+elId+' svg')
      .datum(chartData)
      .call(chart);

    nv.utils.windowResize(function() { chart.update() });
        return chart;
    });
}
