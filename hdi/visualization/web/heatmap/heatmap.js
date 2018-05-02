//Qt Connection
  document.getElementById("ErrorBox").innerHTML = "Hello world!";
/*
var isQtAvailable = true
try {
  Qt.sgnSetData.connect(setData);
} catch (error) {
  document.getElementById("ErrorBox").innerHTML = "Cannot connect to Qt!";
  isQtAvailable = false;
}
// pipe errors to log
window.onerror = function (msg, url, num) {
     logError("Error: " + msg + "\nURL: " + url + "\nLine: " + num);
};

// auto log for Qt and console
function logError(logtext) {
   if (isQtAvailable) {
    Qt.onJsError(logtext.toString());
   }
   else {
    console.log(logtext);
   }
}
Qt.onJsLog("Widget up and running...");


function setData(data_csv){
  Qt.onJsLog("Data changed...");
  var data = d3.csv.parse(data_csv);
}


var width = 600;
var height = 800;
var x_label_space = 30;
var y_label_space = 30;
var max_x_elems = 1;
var max_y_elems = 9;
var gridSize = Math.min(Math.floor((width-y_label_space) / max_x_elems), Math.floor((height-x_label_space) / max_y_elems));
var legendElementWidth = gridSize*2;
var buckets = 9;
var colors = ["#ffffd9","#edf8b1","#c7e9b4","#7fcdbb","#41b6c4","#1d91c0","#225ea8","#253494","#081d58"]; // alternatively colorbrewer.YlGnBu[9]

var y_labels = ["Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"];
var x_labels = ["1a", "2a", "3a", "4a", "5a", "6a", "7a", "8a", "9a", "10a", "11a", "12a", "1p", "2p", "3p", "4p", "5p", "6p", "7p", "8p", "9p", "10p", "11p", "12p"];
var data = "data_3.tsv";

//  document.getElementById("ErrorBox").innerHTML = data;

//Draw Y labels
var drawYLabels = function(){
  var svg = d3.select("#chart").select("svg")
  console.log("Draw y labels");
  var yLabels = svg.selectAll(".xLabel")
    .data(y_labels)
    .enter().append("text")
    .text(function (d) { return d; })
    .attr("x", function(d, i) { return y_label_space-6; })
    .attr("y", function (d, i) { return (i+0.6) * gridSize + x_label_space ; })
    .style("text-anchor", "end")
    .attr("class", function (d, i) { return ((i >= 0 && i <= 4) ? "dayLabel mono axis axis-workweek" : "dayLabel mono axis"); });
}

//Draw X labels
var drawXLabels = function(){
  var svg = d3.select("#chart").select("svg")
  console.log("Draw x labels");
  var xLabels = svg.selectAll(".xLabel")
      .data(x_labels)
      .enter().append("text")
      .text(function(d) { return d; })
      .attr("x", function(d, i) { return (i+0.4) * gridSize + y_label_space; })
      .attr("y", function(d, i) { return x_label_space-6; })
      .attr("class", function(d, i) { return ((i >= 7 && i <= 16) ? "timeLabel mono axis axis-worktime" : "timeLabel mono axis"); });
}

//Draw heatmap
var heatmapChart = function(tsvFile) {
  d3.tsv(tsvFile,
  function(d) {
    return {
    y_label: +d.y_label,
    x_label: +d.x_label,
    value: +d.value,
    confidence: +d.confidence
    };
  },
  function(error, data) {
    var svg = d3.select("#chart").select("svg")
    var colorScale = d3.scale.quantile()
      .domain([0, buckets - 1, d3.max(data, function (d) { return d.value; })])
      .range(colors);

    var cards = svg.selectAll(".x_label")
      .data(data, function(d) {return d.y_label+':'+d.x_label;});

    cards.append("title");

    cards.enter().append("rect")
      .attr("x", function(d) { return (d.x_label - 1) * gridSize + (gridSize * (1-d.confidence) * 0.5) + y_label_space; })
      .attr("y", function(d) { return (d.y_label - 1) * gridSize + (gridSize * (1-d.confidence) * 0.5) + x_label_space; })
      .attr("rx", 4)
      .attr("ry", 4)
      .attr("class", "x_label bordered")
      .attr("width", function(d) { return gridSize * d.confidence; })
      .attr("height", function(d) { return gridSize * d.confidence; })
      .style("fill", function(d) { return colorScale(d.value); })
    
    cards.exit().remove();
  });  
};

//Draw full visualization
var drawVisualization = function (){
  d3.select("#chart").select("svg").remove();
  d3.select("#chart").append("svg")
    .attr("width", width)
    .attr("height", height);

  drawYLabels();
  drawXLabels();
//      heatmapChart(data);
}

drawVisualization();
*/
