'use strict';

const lastUpdated = document.querySelector("#timestamp");
const pump = document.querySelector("#pump");
const moisture0 = document.querySelector("#moisture0");
const moisture1 = document.querySelector("#moisture1");
const moisture2 = document.querySelector("#moisture2");
const moisture3 = document.querySelector("#moisture3");
const valve0 = document.querySelector("#valve0");
const valve1 = document.querySelector("#valve1");
const valve2 = document.querySelector("#valve2");
const valve3 = document.querySelector("#valve3");
const trigger0 = document.querySelector("#trigger0");
const trigger1 = document.querySelector("#trigger1");
const trigger2 = document.querySelector("#trigger2");
const trigger3 = document.querySelector("#trigger3");


const water0 = document.querySelector("#water0");
const water1 = document.querySelector("#water1");
const water2 = document.querySelector("#water2");
const water3 = document.querySelector("#water3");
const configure0 = document.querySelector("#configure0");
const configure1 = document.querySelector("#configure1");
const configure2 = document.querySelector("#configure2");
const configure3 = document.querySelector("#configure3");

//chart data
const dataMinute0 = [];
const dataMinute1 = [];
const dataMinute2 = [];
const dataMinute3 = [];
const dataHour0 = [];
const dataHour1 = [];
const dataHour2 = [];
const dataHour3 = [];
const dataDay0 = [];
const dataDay1 = [];
const dataDay2 = [];
const dataDay3 = [];
const dataWeek0 = [];
const dataWeek1 = [];
const dataWeek2 = [];
const dataWeek3 = [];


const configurationModal = document.querySelector(".configure-modal-container");
const triggerToggle = configurationModal.querySelector("#trigger");
const configurationConfirm = configurationModal.querySelector("#configure-confirm");
const configurationDiscard = configurationModal.querySelector("#configure-discard");
const moistureLower = configurationModal.querySelector("#moisture-lower");
const moistureUpper = configurationModal.querySelector("#moisture-upper");
const waterTime = configurationModal.querySelector("#water-time");

const canvas0 = document.querySelector("#chart0");
const canvas1 = document.querySelector("#chart1");
const canvas2 = document.querySelector("#chart2");
const canvas3 = document.querySelector("#chart3");

const minute0 = document.querySelector("#chart0minute");
const minute1 = document.querySelector("#chart1minute");
const minute2 = document.querySelector("#chart2minute");
const minute3 = document.querySelector("#chart3minute");

const hour0 = document.querySelector("#chart0hour");
const hour1 = document.querySelector("#chart1hour");
const hour2 = document.querySelector("#chart2hour");
const hour3 = document.querySelector("#chart3hour");

const day0 = document.querySelector("#chart0day");
const day1 = document.querySelector("#chart1day");
const day2 = document.querySelector("#chart2day");
const day3 = document.querySelector("#chart3day");

const week0 = document.querySelector("#chart0week");
const week1 = document.querySelector("#chart1week");
const week2 = document.querySelector("#chart2week");
const week3 = document.querySelector("#chart3week");

const sensors = [ moisture0, moisture1, moisture2, moisture3, valve0, valve1, valve2, valve3, trigger0, trigger1, trigger2, trigger3 ];
const keys = [ "a0", "a1", "a2", "a3", "relay0", "relay1", "relay2", "relay3", "trigger0", "trigger1", "trigger2", "trigger3" ];

const configurationMap = new Map();

let currentReadings = undefined;
let lastTimestamp = undefined;




const trap = (ele) => {
	document.activeElement.blur();
	window.focused = ele;
	window.focusable = ele.querySelectorAll('button:not([disabled]), input[type="text"]:not([disabled]), input[type="time"]:not([disabled])');
};

const untrap = (ele) => {
	if(window.focused !== ele)
		return;
	window.focused = undefined;
	window.focusable = undefined;
};

const initChart = (canvas, chartData) => {
	const dateMin = new Date(0);
	dateMin.setMilliseconds(0);
	const date = new Date();
	date.setTime(dateMin.valueOf() + 60000);

	let data = {
		datasets: [{
			data: chartData
		}],
		tension: 0.1
	};

	let config = {
		type: "line",
		data: data,
		options: {
			backgroundColor: "#282828",
			borderColor: "#EEE",
			maintainAspectRatio: false,
			plugins: {
				legend: {
					display: false
				}
			},
			animation: {
				duration: 0
			},
			elements: {
				point: {
					radius: 0
				}
			},
			scales: {
				y: {
					beginAtZero: true,
					grid: {
						color: "#444"
					},
					min: 0,
					max: 100
				},
				x: {
					type: "time",
					min: dateMin.valueOf(),
					max: date.valueOf(),
					time: {
						unit: "second",
						displayFormats: {
							second: "mm:ss",
							minute: "hh:mm",
							hour: "hh:mm",
							week: "DDD"
						}
					},
					grid: {
						color: "#444"
					},
					ticks: {
						maxTicksLimit: 12
					}
				}
			}
		}
	};

	let chart = new Chart(canvas, config);

	return [data, config, chart];
};

const [chartData0, chartConfig0, chart0] = initChart(canvas0, dataMinute0);
const [chartData1, chartConfig1, chart1] = initChart(canvas1, dataMinute1);
const [chartData2, chartConfig2, chart2] = initChart(canvas2, dataMinute2);
const [chartData3, chartConfig3, chart3] = initChart(canvas3, dataMinute3);

const chartDataMinute = [ dataMinute0, dataMinute1, dataMinute2, dataMinute3 ];
const chartDataHour = [ dataHour0, dataHour1, dataHour2, dataHour3 ];
const chartDataDay = [ dataDay0, dataDay1, dataDay2, dataDay3 ];
const chartDataWeek = [ dataWeek0, dataWeek1, dataWeek2, dataWeek3 ];
const chartConfig = [ chartConfig0, chartConfig1, chartConfig2, chartConfig3 ];
const charts = [ chart0, chart1, chart2, chart3 ];
const canvas = [ canvas0, canvas1, canvas2, canvas3 ];

const updateChart = (chartIndex, dataPoint) => {
	let dataMinute = chartDataMinute[chartIndex];
	let truncate = false;
	dataMinute.unshift(dataPoint);
	if(dataMinute.length % 30 === 0)
	{
		if(dataMinute.length > 60)
			dataMinute.length = 60;
		truncate = true;
	}
	if(canvas[chartIndex].classList.contains("minute"))
	{
		let min = new Date();
		let rounded = new Date(dataPoint.x).setMilliseconds(0);
		min.setTime(rounded.valueOf() - 60000);
		chartConfig[chartIndex].options.scales.x.min = min;
		chartConfig[chartIndex].options.scales.x.max = new Date(dataPoint.x);
		charts[chartIndex].update();
	}
	if(!truncate)
		return;
	truncate = false;
	let dataHour = chartDataHour[chartIndex];
	let newData = { ...dataPoint };
	let i = 1;
	while(i < 30)
		newData.y += dataMinute[i++].y;
	newData.y /= 30;
	newData.y = Math.floor(newData.y);
	dataHour.unshift(newData);
	if(dataHour.length % 24 === 0)
	{
		if(dataHour.length > 120)
			dataMinute.length = 120;
		truncate = true;
	}
	if(canvas[chartIndex].classList.contains("hour"))
	{
		let min = new Date();
		chartConfig[chartIndex].options.scales.x.max = new Date(newData.x - (newData.x % 60000) + 60000);
		min.setTime(chartConfig[chartIndex].options.scales.x.max.valueOf() - 3600000);
		chartConfig[chartIndex].options.scales.x.min = min;
		charts[chartIndex].update();
	}
	if(!truncate)
		return;
	truncate = false;
	let dataDay = chartDataDay[chartIndex];
	i = 1;
	while(i < 24)
		newData.y += dataHour[i++].y;
	newData.y /= 24;
	newData.y = Math.floor(newData.y);
	dataDay.unshift(newData);
	if(dataDay.length % 7 === 0)
	{
		if(dataDay.length > 120)
			dataDay.length = 120;
		truncate = true;
	}
	if(canvas[chartIndex].classList.contains("day"))
	{
		let min = new Date();
		chartConfig[chartIndex].options.scales.x.max = new Date(newData.x - (newData.x % 3600000) + 3600000);
		min.setTime(chartConfig[chartIndex].options.scales.x.max.valueOf() - 86400000);
		chartConfig[chartIndex].options.scales.x.min = min;
		charts[chartIndex].update();
	}
	if(!truncate)
		return;
	let dataWeek = chartDataWeek[chartIndex];
	i = 1;
	while(i < 7)
		newData.y += dataDay[i++].y;
	newData.y /= 7;
	newData.y = Math.floor(newData.y);
	dataWeek.unshift(newData);
	if(dataWeek.length > 120)
		dataWeek.length = 120;
	if(canvas[chartIndex].classList.contains("week"))
	{
		let min = new Date();
		chartConfig[chartIndex].options.scales.x.max = new Date(newData.x - (newData.x % 86400000) + 108000000);
		min.setTime(chartConfig[chartIndex].options.scales.x.max.valueOf() - 691200000);
		chartConfig[chartIndex].options.scales.x.min = min;
		charts[chartIndex].update();
	}
};

const getChartHistory = async () => {
	try
	{
		const response = await fetch("/api/history");
		const json = await response.json();
		let i = 0;
		json.forEach((element) => {
			chartDataMinute[i].length = 0;
			chartDataMinute[i].push(...element.minute);
			chartDataHour[i].length = 0;
			chartDataHour[i].push(...element.hour);
			chartDataDay[i].length = 0;
			chartDataDay[i].push(...element.day);
			chartDataWeek[i].length = 0;
			chartDataWeek[i].push(...element.week);
			i++;
		});
	}
	catch(ex)
	{
		console.log(ex);
		setTimeout(getChartHistory, 1000);
	}
};

getChartHistory();


const updateSensors = async () => {
	try
	{
		const response = await fetch("/api/sensors");
		const json = await response.json();
		currentReadings = json;
		const timestamp = new Date(json.timestamp);
		if(timestamp.valueOf() === lastTimestamp)
		{
			setTimeout(updateSensors, 250);
			return;
		}
		lastTimestamp = timestamp.valueOf();
		if(json.timestamp !== null)
			lastUpdated.innerText = timestamp.toLocaleString("en-us", { hourCycle: "h23"});
		else
			lastUpdated.innerText = "null";
		if(json.pump !== null)
		{
			if(json.pump)
				pump.innerText = "On";
			else
				pump.innerText = "Off";
		}
		else
		{
			pump.innerText = "null";
		}
		let sensorText = undefined;
		let i = 0;
		sensors.forEach((sensor) => {
			sensorText = sensor.querySelector(".sensor-value-text");
			if(sensorText === null)
			{
				if(json[keys[i]] === null)
				{
					sensor.innerText = "null";
				}
				else
				{
					if(json[keys[i]] === 0)
						sensor.innerText = "MOISTURE";
					else
						sensor.innerText = "TIME";
				}
			}
			else if(sensorText.classList.contains("percent"))
			{
				if(json[keys[i]] === null)
				{
					sensorText.innerText = "-1%";
				}
				else
				{
					sensorText.innerText = json[keys[i]] + "%";
					updateChart(i, { x: timestamp.valueOf(), y: json[keys[i]]});
				}
			}
			else
			{
				if(json[keys[i]] === null)
				{
					sensorText.innerText = "null";
				}
				else
				{
					if((sensorText.innerText === "Open" && json[keys[i]] === 0) || (sensorText.innerText === "Closed" && json[keys[i]] === 1))
					{
						sensor.classList.toggle("open");
						sensor.classList.toggle("closed");
					}
					else if(sensorText.innerText === "null" || sensorText.innerText === "-")
					{
						if(json[keys[i]])
						{
							sensor.classList.toggle("open");
							sensor.classList.toggle("closed");
						}
					}
					if(json[keys[i]] === 1)
						sensorText.innerText = "Open";
					else
						sensorText.innerText = "Closed";
				}
			}
			if(sensorText !== null)
				sensorText.classList.remove("init");
			i++;
		});
	}
	catch(ex)
	{
		console.log(ex);
	}
	setTimeout(updateSensors, 1250);
};

setTimeout(updateSensors, 1000);


const sendWaterCommand = async (ev, target) => {
	let param = undefined;
	setTimeout(()=> target.blur(), 10);
	switch(target.id)
	{
		case "water0":
			param = "0";
			break;
		case "water1":
			param = "1";
			break;
		case "water2":
			param = "2";
			break;
		case "water3":
			param = "3";
			break;
		default:
			break;
	}
	if(param === undefined)
		return;
	try
	{
		let response = await fetch("/api/queue", {
			method: "POST",
			headers: {
				"Content-Type": "application/json"
			},
			body: JSON.stringify({
				command: "water",
				param: param
			})
		});
		if(response.status !== 200)
		{
			console.log("POST to /api/queue received response status code " + response.status); 
			return;
		}
		let json = await response.json();
		if(json?.status === undefined || json.status !== "ok")
		{
			console.log("POST to /api/queue received an unexpected response");
		}
	}
	catch(ex)
	{
		console.log(ex);
	}
	return;
};

const changeChartTimescale = (ev, target) => {
	if(target.classList.contains("selected"))
		return;
	let previous = target.closest(".chart-controls").querySelector(".selected");
	previous.classList.remove("selected");
	target.classList.add("selected");
	let index = parseInt(target.id[5]);
	let data = undefined;
	let min = new Date();
	let max = undefined;
	let sub = 0;
	let mod = 0;
	let adj = 0;
	let rounded = undefined;
	let newClass = undefined;
	let unit = undefined;
	let ticks = undefined;
	switch(target.id[6])
	{
		case 'm':
			data = chartDataMinute[index];
			sub = 60000;
			mod = 1;
			adj = 0;
			newClass = "minute";
			unit = "second";
			ticks = 12;
			break;
		case 'h':
			data = chartDataHour[index];
			sub = 3600000;
			mod = 60000;
			adj = 60000;
			newClass = "hour";
			unit = "minute";
			ticks = 11;
			break;
		case 'd':
			data = chartDataDay[index];
			sub = 86400000;
			mod = 3600000;
			adj = 3600000;
			newClass = "day";
			unit = "hour";
			ticks = 12;
			break;
		case 'w':
			data = chartDataWeek[index];
			sub = 691200000;
			mod = 86400000;
			adj = 108000000;
			newClass = "week";
			unit = "day";
			ticks = 8;
			break;
	}
	if(data.length > 0)
	{
		max = new Date(data[0].x - (data[0].x % mod) + adj);
		min.setTime(max.valueOf() - sub);
	}
	else
	{
		let now = Date.now();
		max = new Date(now - (now % mod) + adj).setMilliseconds(0);
		min.setTime(max.valueOf() - sub);
	}
	chartConfig[index].options.scales.x.min = min;
	chartConfig[index].options.scales.x.max = max;
	chartConfig[index].options.scales.x.time.unit = unit;
	chartConfig[index].options.scales.x.ticks.maxTicksLimit = ticks;
	charts[index].data.datasets[0].data = data;
	canvas[index].classList.remove(previous.id.substring(6));
	canvas[index].classList.add(newClass);
	charts[index].update();
};

const sendModeCommand = async (ev, target) => {
	setTimeout(()=> target.blur(), 10);
	configurationModal.classList.remove("active");
	let payload = {
		command: "mode",
		param: configurationModal.dataset.param
	};
	if(configurationModal.classList.contains("moisture"))
	{
		payload.trigger = "moisture";
		payload.open = parseInt(moistureLower.value);
		payload.close = parseInt(moistureUpper.value);
	}
	else if(configurationModal.classList.contains("time"))
	{
		payload.trigger = "time";
		let temp = waterTime.value.split(':');
		payload.hour = parseInt(temp[0]);
		payload.minute = parseInt(temp[1]);
	}
	else
		return;
	console.log(payload);
	try
	{
		let response = await fetch("/api/queue", {
			method: "POST",
			headers: {
				"Content-Type": "application/json"
			},
			body: JSON.stringify(payload)
		});
		if(response.status !== 200)
		{
			console.log("POST to /api/queue received response status code " + response.status); 
			return;
		}
		let json = await response.json();
		if(json?.status === undefined || json.status !== "ok")
		{
			console.log("POST to /api/queue received an unexpected response");
		}
	}
	catch(ex)
	{
		console.log(ex);
	}
};

const showConfigurationModal = (ev, target) => {
	setTimeout(()=> target.blur(), 10);
	let dashboardCard = target.closest(".dashboard-card");
	configurationModal.dataset.param = dashboardCard.id[1];
	configurationModal.querySelector(".configure-modal-title").innerHTML = dashboardCard.querySelector(".card-title").innerHTML;
	if(currentReadings === undefined || currentReadings.timestamp === null)
	{
		triggerToggle.dataset.selected = "moisture";
		configurationModal.classList.remove("time");
		configurationModal.classList.add("moisture");
		triggerToggle.querySelector(".toggle-selected-text").innerText = "Moisture";
		moistureLower.value = 30;
		moistureUpper.value = 55;
		waterTime.value = "00:00";
	}
	else
	{
		if(currentReadings["trigger" + dashboardCard.id[1]] === 0)
		{
			triggerToggle.dataset.selected = "moisture";
			configurationModal.classList.remove("time");
			configurationModal.classList.add("moisture");
			triggerToggle.querySelector(".toggle-selected-text").innerText = "Moisture";
		}
		else
		{
			triggerToggle.dataset.selected = "time";
			configurationModal.classList.remove("moisture");
			configurationModal.classList.add("time");
			triggerToggle.querySelector(".toggle-selected-text").innerText = "Time";
		}
		moistureLower.value = currentReadings["lower" + dashboardCard.id[1]];
		moistureUpper.value = currentReadings["upper" + dashboardCard.id[1]];
		waterTime.value = "" + (currentReadings["hour" + dashboardCard.id[1]] < 10 ? "0" : "") + currentReadings["hour" + dashboardCard.id[1]] + ":" + (currentReadings["minute" + dashboardCard.id[1]] < 10 ? "0" : "") + currentReadings["minute" + dashboardCard.id[1]];
	}
	configurationModal.classList.add("active");
	trap(configurationModal);
};

const hideConfigurationModal = (ev, target) => {
	setTimeout(()=> target.blur(), 10);
	configurationModal.classList.remove("active");
	untrap(configurationModal);
};

const toggleTrigger = (ev, target) => {
	if(target.dataset.selected === "moisture")
	{
		target.dataset.selected = "time";
		target.querySelector(".toggle-selected-text").innerText = "Time";
		configurationModal.classList.remove("moisture");
		configurationModal.classList.add("time");
		configurationConfirm.classList.add("click");
	}
	else
	{
		target.dataset.selected = "moisture";
		target.querySelector(".toggle-selected-text").innerText = "Moisture";
		configurationModal.classList.remove("time");
		configurationModal.classList.add("moisture");
		if(parseInt(moistureLower.value) >= parseInt(moistureUpper.value))
			configurationConfirm.classList.remove("click");
		else
			configurationConfirm.classList.add("click");
	}
};

const clickHandlerMap = new Map();
clickHandlerMap.set(water0, sendWaterCommand);
clickHandlerMap.set(water1, sendWaterCommand);
clickHandlerMap.set(water2, sendWaterCommand);
clickHandlerMap.set(water3, sendWaterCommand);
clickHandlerMap.set(configure0, showConfigurationModal);
clickHandlerMap.set(configure1, showConfigurationModal);
clickHandlerMap.set(configure2, showConfigurationModal);
clickHandlerMap.set(configure3, showConfigurationModal);
clickHandlerMap.set(minute0, changeChartTimescale);
clickHandlerMap.set(minute1, changeChartTimescale);
clickHandlerMap.set(minute2, changeChartTimescale);
clickHandlerMap.set(minute3, changeChartTimescale);
clickHandlerMap.set(hour0, changeChartTimescale);
clickHandlerMap.set(hour1, changeChartTimescale);
clickHandlerMap.set(hour2, changeChartTimescale);
clickHandlerMap.set(hour3, changeChartTimescale);
clickHandlerMap.set(day0, changeChartTimescale);
clickHandlerMap.set(day1, changeChartTimescale);
clickHandlerMap.set(day2, changeChartTimescale);
clickHandlerMap.set(day3, changeChartTimescale);
clickHandlerMap.set(week0, changeChartTimescale);
clickHandlerMap.set(week1, changeChartTimescale);
clickHandlerMap.set(week2, changeChartTimescale);
clickHandlerMap.set(week3, changeChartTimescale);
clickHandlerMap.set(configurationConfirm, sendModeCommand);
clickHandlerMap.set(configurationDiscard, hideConfigurationModal);
clickHandlerMap.set(triggerToggle, toggleTrigger);

document.addEventListener("click", (ev) => {
	let target = ev.target;
	if(target === document.body)
		return;
	while(1)
	{
		if(target.classList.contains("click"))
		{
			let callback = clickHandlerMap.get(target);
			if(callback !== undefined)
				callback(ev, target);
		}
		target = target.parentElement?.closest(".click");
		if(target !== null && target !== undefined)
			continue;
		break;
	}
});

const checkMoistureBounds = (ev, target) => {
	console.log(target.value);
	if(parseInt(target.value) < 0 || parseInt(target.value) === 0 || isNaN(parseInt(target.value)))
		target.value = 0;
	else if(target.value > 100)
		target.value = 100;
	else
		target.value = parseInt(target.value);
	if(parseInt(moistureLower.value) >= parseInt(moistureUpper.value))
		configurationConfirm.classList.remove("click");
	else
		configurationConfirm.classList.add("click");
};

const checkWaterTime = (ev, target) => {
	if(waterTime.value === "")
		waterTime.value = "12:00";
};


const inputHandlerMap = new Map();
inputHandlerMap.set(moistureLower, checkMoistureBounds);
inputHandlerMap.set(moistureUpper, checkMoistureBounds);
inputHandlerMap.set(waterTime, checkWaterTime);

document.addEventListener("input", (ev) => {
	let target = ev.target;
	if(target === document.body)
		return;
	while(1)
	{
		if(target.classList.contains("input"))
		{
			let callback = inputHandlerMap.get(target);
			if(callback !== undefined)
				callback(ev, target);
		}
		target = target.parentElement?.closest(".input");
		if(target !== null && target !== undefined)
			continue;
		break;
	}
});

window.addEventListener("keydown", (ev) => {
	let isTab = (ev.key === 'Tab' || ev.keyCode === 9);
	if(!isTab)
		return;
	if(window?.focused === undefined)
		return;
	if(Array.from(window.focusable).includes(document.activeElement) === false)
	{
		window.focusable[0].focus();
		ev.preventDefault();
		return;
	}
	if(ev.shiftKey)
	{
		if(document.activeElement === window.focusable[0])
		{
			window.focusable[window.focusable.length - 1].focus();
			ev.preventDefault();
		}
		return;
	}
	if(document.activeElement === window.focusable[window.focusable.length - 1])
	{
		window.focusable[0].focus();
		ev.preventDefault();
	}
});
