'use strict'

const express = require("express");
const https = require("https");
const cookieParser = require("cookie-parser");
const fs = require("fs");

const config = require("./config.json");

const commands = [];

const sensors = {
	"a0": null,
	"relay0": null,
	"trigger0": 0,
	"lower0": 30,
	"upper0": 55,
	"hour0": 0,
	"minute0": 0,
	"a1": null,
	"relay1": null,
	"trigger1": 0,
	"lower1": 30,
	"upper1": 55,
	"hour1": 0,
	"minute1": 0,
	"a2": null,
	"relay2": null,
	"trigger2": 0,
	"lower2": 30,
	"upper2": 55,
	"hour2": 0,
	"minute2": 0,
	"a3": null,
	"relay3": null,
	"trigger3": 0,
	"lower3": 30,
	"upper3": 55,
	"hour3": 0,
	"minute3": 0,
	"pump": null,
	"timestamp": null
};

//ORDER OF ELEMENTS IN THIS ARRAY IS CRITICAL - DO NOT REARRANGE
const keys = [
	"a0",
	"relay0",
	"trigger0",
	"lower0",
	"upper0",
	"hour0",
	"minute0",
	"a1",
	"relay1",
	"trigger1",
	"lower1",
	"upper1",
	"hour1",
	"minute1",
	"a2",
	"relay2",
	"trigger2",
	"lower2",
	"upper2",
	"hour2",
	"minute2",
	"a3",
	"relay3",
	"trigger3",
	"lower3",
	"upper3",
	"hour3",
	"minute3",
	"pump"
];

//ORDER OF ELEMENTS IN THIS ARRAY IS CRITICAL - DO NOT REARRANGE
const confKeys = [
	"trigger0",
	"trigger1",
	"trigger2",
	"trigger3",
	"lower0",
	"lower1",
	"lower2",
	"lower3",
	"upper0",
	"upper1",
	"upper2",
	"upper3",
	"hour0",
	"hour1",
	"hour2",
	"hour3",
	"minute0",
	"minute1",
	"minute2",
	"minute3"
];

const validCommands = new Map();
validCommands.set("water", [ "0", "1", "2", "3", "all" ]);
validCommands.set("sync", null);
validCommands.set("mode", [ "0", "1", "2", "3", "all" ]);

const validTriggers = new Map();
validTriggers.set("moisture", null);
validTriggers.set("time", null);

const configurationKeys = new Map();
configurationKeys.set("trigger0", null);
configurationKeys.set("trigger1", null);
configurationKeys.set("trigger2", null);
configurationKeys.set("trigger3", null);
configurationKeys.set("lower0", null);
configurationKeys.set("lower1", null);
configurationKeys.set("lower2", null);
configurationKeys.set("lower3", null);
configurationKeys.set("upper0", null);
configurationKeys.set("upper1", null);
configurationKeys.set("upper2", null);
configurationKeys.set("upper3", null);
configurationKeys.set("hour0", null);
configurationKeys.set("hour1", null);
configurationKeys.set("hour2", null);
configurationKeys.set("hour3", null);
configurationKeys.set("minute0", null);
configurationKeys.set("minute1", null);
configurationKeys.set("minute2", null);
configurationKeys.set("minute3", null);

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
const chartDataMinute = [ dataMinute0, dataMinute1, dataMinute2, dataMinute3 ];
const chartDataHour = [ dataHour0, dataHour1, dataHour2, dataHour3 ];
const chartDataDay = [ dataDay0, dataDay1, dataDay2, dataDay3 ];
const chartDataWeek = [ dataWeek0, dataWeek1, dataWeek2, dataWeek3 ];

const updateChartData = (chartIndex, dataPoint) => {
	let dataMinute = chartDataMinute[chartIndex];
	let truncate = false;
	dataMinute.unshift(dataPoint);
	if(dataMinute.length % 30 === 0)
	{
		if(dataMinute.length > 60)
			dataMinute.length = 60;
		truncate = true;
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
};

const saveConfiguration = () => {
	let arduinoConf = {};
	keys.forEach((key) => {
		if(!configurationKeys.has(key))
			return;
		arduinoConf[key] = sensors[key];
		return;
	});
	fs.open("arduino.conf", "w", (err, fd) => {
		if(err)
		{
			console.error(err);
			return;
		}
		fs.write(fd, JSON.stringify(arduinoConf), (err, nbytes) => {
			if(err)
			{
				console.error(err);
				return;
			}
			fs.close(fd, (err) => {
				if(err)
				{
					console.log(err);
					return;
				}
				console.log("Running configuration written successfully (arduino.conf)");
				return;
			});
		});
	});
};

const app = express();

app.set("etag", false);
app.set("x-powered-by", false);

app.use(cookieParser());
app.use(express.json());

app.use((req, res, next) => {
	res.removeHeader("Date");
	next();
});

app.get("/", (req, res) => {
	res.redirect("/index");
	return;
});

app.use(express.static("./static", { "extensions": [ "html" ] }));

let publicViews = fs.readdirSync("./views");
publicViews.forEach(file => {
	app.get("/" + file.split('.')[0], (req, res) => {
		res.sendFile(file, {
			root: "./views"
		});
		return;
	});
});

app.route("/elecrow/report")
	.get((req, res) => {
		const ts = new Date(Date.now()).toUTCString();
		let name = 'a';
		let json = {};
		json.command = "conf";
		confKeys.forEach((key) => {
			json[name] = "" + sensors[key];
			name = String.fromCharCode(name.charCodeAt(0) + 1);
		});
		res.status(200).json(json);
		commands.length = 0;
		commands.push({
			id: "sync"
		});
		console.log("Running configuration sent successfully (arduino.conf)");
		return;
	})
	.post((req, res) => {
		const ts = new Date(Date.now()).toUTCString();
		if(req.body?.a === undefined || Array.isArray(req.body.a) === false || req.body.a.length !== 29)
		{
			res.status(400).json({});
			return;
		}
		let i = 0;
		let configurationChanged = false;
		sensors.timestamp = Date.now();
		keys.forEach((key) => {
			if(req.body.a[i] === undefined)
				return;
			if(!configurationChanged && sensors[key] !== req.body.a[i] && configurationKeys.has(key))
				configurationChanged = true;
			sensors[key] = req.body.a[i++];
		});
		for(i = 0; i < 4; i++)
			updateChartData(i, { x: sensors.timestamp, y: sensors["a" + i] });
		const json = {};
		if(commands.length > 0)
		{
			let command = commands.shift();
			json.command = command.id;
			if(command.id === "sync")
			{
				const dt = new Date(Date.now() - 3600 * 1000 * 5); //UTC date to central
				let rtcData = "" + dt.getSeconds()
					+ " " + dt.getMinutes()
					+ " " + dt.getHours()
					+ " " + (dt.getDay() + 1)
					+ " " + dt.getDate()
					+ " " + (dt.getMonth() + 1)
					+ " " + (dt.getYear() - 100);
				json.param = rtcData;
			}
			else
			{
				json.param = command.param;
				json.a = command.a;
				json.b = command.b;
				json.c = command.c;
			}
		}
		res.status(200).json(json);
		if(configurationChanged)
			saveConfiguration();
		return;
	});

app.route("/api/sensors")
	.get((req, res) => {
		res.status(200).json(sensors);
		return;
	});

app.route("/api/history")
	.get((req, res) => {
		let response = [];
		let i = undefined;
		for(i = 0; i < 4; i++)
		{
			response.push({
				minute: chartDataMinute[i],
				hour: chartDataHour[i],
				day: chartDataDay[i],
				week: chartDataWeek[i]
			});
		}
		res.status(200).json(response);
		return;
	});

app.route("/api/queue")
	.get((req, res) => {
		res.status(200).json({
			queue: commands
		});
		return;
	})
	.post((req, res) => {
		if(req.body?.command === undefined || typeof req.body.command !== "string")
		{
			res.status(400).json({
				status: "error"
			});
			return;
		}
		let mapEntry = validCommands.get(req.body.command);
		if(mapEntry === undefined)
		{
			res.status(400).json({
				status: "error"
			});
			return;
		}
		switch(req.body.command)
		{
			case "water":
				if(req.body?.param === undefined || typeof req.body.param !== "string" || mapEntry.includes(req.body.param) === false)
				{
					res.status(400).json({
						status: "error"
					});
					return;
				}
				commands.push({
					id: req.body.command,
					param: req.body.param
				});
				break;
			case "sync":
				commands.push({
					id: req.body.command,
				});
				break;
			case "mode":
				if(req.body?.param === undefined || typeof req.body.param !== "string" || mapEntry.includes(req.body.param) === false)
				{
					res.status(400).json({
						status: "error"
					});
					return;
				}
				if(req.body?.trigger === undefined || typeof req.body.trigger !== "string")
				{
					res.status(400).json({
						status: "error"
					});
					return;
				}
				let triggerEntry = validTriggers.get(req.body.trigger);
				if(triggerEntry === undefined)
				{
					res.status(400).json({
						status: "error"
					});
					return;
				}
				switch(req.body.trigger)
				{
					case "moisture":
						if(req.body?.open === undefined || typeof req.body.open !== "number" || req.body?.close === undefined || typeof req.body.close !== "number")
						{
							res.status(400).json({
								status: "error"
							});
							return;
						}
						if(req.body.open > 100 || req.body.open < 0 || req.body.close > 100 || req.body.close < 0 || req.body.open >= req.body.close)
						{
							res.status(400).json({
								status: "error"
							});
							return;
						}
						commands.push({
							id: req.body.command,
							param: req.body.param,
							a: "0",
							b: "" + Math.floor(req.body.open),
							c: "" + Math.floor(req.body.close)
						});
						break;
					case "time":
						if(req.body?.hour === undefined || typeof req.body.hour !== "number" || req.body?.minute === undefined || typeof req.body.minute !== "number")
						{
							res.status(400).json({
								status: "error"
							});
							return;
						}
						if(req.body.hour > 23 || req.body.hour < 0 || req.body.minute > 59 || req.body.minute < 0)
						{
							res.status(400).json({
								status: "error"
							});
							return;
						}
						commands.push({
							id: req.body.command,
							param: req.body.param,
							a: "1",
							b: "" + Math.floor(req.body.hour),
							c: "" + Math.floor(req.body.minute)
						});
						break;
				}
				break;
		}
		res.status(200).json({
			status: "ok"
		});
	});

/* CATCH ALL ROUTE */

app.use((req, res) => {
	res.status(200).send('WATCH YOUR BACK ;)');
	console.log("[%s %s UTC] Unexpected packet received! @ %s", (new Date(Date.now())).toDateString(), (new Date(Date.now())).toLocaleTimeString(), req.originalUrl);
	//process.stdout.write("\r" + (++i) + " PACKETS RECEIVED." + req.originalUrl);
	return;
});

/* END OF CATCH ALL ROUTE */

//one time anonymous init function to load arduino.conf
(() => {
	try
	{
		let runningConfiguration = fs.readFileSync("arduino.conf");
		let json = JSON.parse(runningConfiguration.toString());
		keys.forEach((key) => {
			if(json?.[key] === undefined || sensors?.[key] === undefined)
				return;
			sensors[key] = json[key];
		});
		console.log("Running configuration loaded successfully (arduino.conf)");
	}
	catch(ex)
	{
		console.error("Running configuration not found (arduino.conf) - Using defaults.");
	}
})();

const server = app.listen(
	{
		host: config.server.host,
		port: config.server.port
	}, () => {
		console.log(`\nServer ready at ${config.server.host}:${config.server.port}`);
		//sendEmail("4792835439@vtext.com", null, "Server Online!");
	});

const exitProg = async () => {
	console.log("\nGraceful shutdown initiated.");
	server.unref();
	process.exit(0);
};

process.on('SIGTERM', () => {
	exitProg();
});

process.on('SIGINT', () => {
	exitProg();
});
