[Overview](_OVERVIEW.md) 

## REST API endpoint: config

`http://IP-ADDRESS/config`


Get and set process related configuration parameter. The data is provided / needs to be 
provided in JSON notation.<br> Parameter description for every single parameter is 
located at github repository (`docs/Configuration/Parameter`) or can be displayed on 
WebUI configuration page (question mark symbol next to each parameter). 

- JSON: `/config`
- HTML: `/config?task=reload`

1. Get API name and version:
    - Payload:
      - `/config?task=api_name`
    - Response:
      - Content type: `HTML`
      - Content: HTML query response, e.g. `config:v1`

2. HTML query request to reload configuration and reinit process:
    - Payload:
      - `/config?task=reload`
    - Response:
      - Content type: `HTML`
      - Content: HTML query response

3. Get config in JSON notation (GET handler)
    - Payload:
      - No payload needed
    - Response:
      - Content type: `JSON`
      - Content: JSON response
    - Example: see below

4. Set config in JSON notation (POST handler)
    - Payload:
      - Configuration in JSON notation
      - Setting only a single, some parameter or all parameter is supported
    - Response:
      - POST handler status response
    - Example: see below

```
{
	"config":	{
		"version":	3,
		"lastmodified":	"2024-09-18T00:09:06+0200"
	},
	"operationmode":	{
		"opmode":	-1,
		"automaticprocessinterval":	"1.0",
		"usedemoimages":	false
	},
	"takeimage":	{
		"flashlight":	{
			"flashtime":	2000,
			"flashintensity":	20
		},
		"camera":	{
			"camerafrequency":	20,
			"imagequality":	12,
			"imagesize":	"VGA",
			"brightness":	0,
			"contrast":	0,
			"saturation":	0,
			"sharpness":	0,
			"exposurecontrolmode":	1,
			"autoexposurelevel":	0,
			"manualexposurevalue":	300,
			"gaincontrolmode":	1,
			"manualgainvalue":	0,
			"specialeffect":	0,
			"mirrorimage":	false,
			"flipimage":	false,
			"zoommode":	0,
			"zoomoffsetx":	0,
			"zoomoffsety":	0
		},
		"debug":	{
			"saverawimages":	false,
			"rawimageslocation":	"/log/source",
			"rawimagesretention":	3
		}
	},
	"imagealignment":	{
		"alignmentalgo":	0,
		"searchfield":	{
			"x":	20,
			"y":	20
		},
		"imagerotation":	"0.0",
		"flipimagesize":	false,
		"marker":	[{
				"x":	132,
				"y":	111
			}, {
				"x":	501,
				"y":	131
			}],
		"debug":	{
			"savedebuginfo":	false
		}
	},
	"numbersequences":	{
		"sequence":	[{
				"sequenceid":	0,
				"sequencename":	"main"
			}]
	},
	"digit":	{
		"enabled":	true,
		"model":	"dig-class100_0168_s2_q.tflite",
		"cnngoodthreshold":	"0.00",
		"sequence":	[{
				"sequenceid":	0,
				"sequencename":	"main",
				"roi":	[{
						"x":	195,
						"y":	63,
						"dx":	55,
						"dy":	75
					}, {
						"x":	255,
						"y":	63,
						"dx":	55,
						"dy":	75
					}, {
						"x":	315,
						"y":	63,
						"dx":	55,
						"dy":	75
					}, {
						"x":	375,
						"y":	63,
						"dx":	55,
						"dy":	75
					}, {
						"x":	435,
						"y":	63,
						"dx":	55,
						"dy":	75
					}]
			}],
		"debug":	{
			"saveroiimages":	false,
			"roiimageslocation":	"/log/digit",
			"roiimagesretention":	3
		}
	},
	"analog":	{
		"enabled":	true,
		"model":	"ana-class100_0171_s1_q.tflite",
		"sequence":	[{
				"sequenceid":	0,
				"sequencename":	"main",
				"roi":	[{
						"x":	450,
						"y":	195,
						"dx":	132,
						"dy":	132,
						"ccw":	false
					}, {
						"x":	358,
						"y":	305,
						"dx":	132,
						"dy":	132,
						"ccw":	false
					}, {
						"x":	215,
						"y":	332,
						"dx":	132,
						"dy":	132,
						"ccw":	false
					}, {
						"x":	84,
						"y":	250,
						"dx":	132,
						"dy":	132,
						"ccw":	false
					}]
			}],
		"debug":	{
			"saveroiimages":	false,
			"roiimageslocation":	"/log/analog",
			"roiimagesretention":	3
		}
	},
	"postprocessing":	{
		"sequence":	[{
				"sequenceid":	0,
				"sequencename":	"main",
				"decimalshift":	0,
				"analogdigitsyncvalue":	"9.2",
				"extendedresolution":	true,
				"ignoreleadingnan":	false,
				"checkdigitincreaseconsistency":	false,
				"maxratechecktype":	1,
				"maxrate":	"0.150",
				"allownegativerate":	false,
				"usefallbackvalue":	true,
				"fallbackvalueagestartup":	720
			}],
		"debug":	{
			"savedebuginfo":	false
		}
	},
	"mqtt":	{
		"enabled":	false,
		"uri":	"",
		"maintopic":	"watermeter",
		"clientid":	"watermeter",
		"authmode":	1,
		"username":	"",
		"password":	"",
		"tls":	{
			"cacert":	"",
			"clientcert":	"",
			"clientkey":	""
		},
		"processdatanotation":	0,
		"retainprocessdata":	false,
		"homeassistant":	{
			"discoveryenabled":	true,
			"discoveryprefix":	"homeassistant",
			"statustopic":	"homeassistant/status",
			"metertype":	1,
			"retaindiscovery":	false
		}
	},
	"influxdbv1":	{
		"enabled":	false,
		"uri":	"",
		"database":	"watermeter",
		"authmode":	0,
		"username":	"",
		"password":	"",
		"tls":	{
			"cacert":	"",
			"clientcert":	"",
			"clientkey":	""
		},
		"sequence":	[{
				"sequenceid":	0,
				"sequencename":	"main",
				"measurementname":	"",
				"fieldname":	""
			}]
	},
	"influxdbv2":	{
		"enabled":	false,
		"uri":	"",
		"bucket":	"watermeter",
		"organization":	"",
		"authmode":	1,
		"token":	"",
		"tls":	{
			"cacert":	"",
			"clientcert":	"",
			"clientkey":	""
		},
		"sequence":	[{
				"sequenceid":	0,
				"sequencename":	"main",
				"measurementname":	"",
				"fieldname":	""
			}]
	},
	"gpio":	{
		"customizationenabled":	true,
		"gpiopin":	[{
				"gpionumber":	1,
				"gpiousage":	"restricted: uart0-tx",
				"pinenabled":	false,
				"pinname":	"",
				"pinmode":	"input",
				"capturemode":	"cyclic-polling",
				"inputdebouncetime":	200,
				"pwmfrequency":	5000,
				"exposetomqtt":	false,
				"exposetorest":	false,
				"smartled":	{
					"type":	0,
					"quantity":	1,
					"colorredchannel":	255,
					"colorgreenchannel":	255,
					"colorbluechannel":	255
				},
				"intensitycorrectionfactor":	100
			}, {
				"gpionumber":	3,
				"gpiousage":	"restricted: uart0-rx",
				"pinenabled":	false,
				"pinname":	"",
				"pinmode":	"input",
				"capturemode":	"cyclic-polling",
				"inputdebouncetime":	200,
				"pwmfrequency":	5000,
				"exposetomqtt":	false,
				"exposetorest":	false,
				"smartled":	{
					"type":	0,
					"quantity":	1,
					"colorredchannel":	255,
					"colorgreenchannel":	255,
					"colorbluechannel":	255
				},
				"intensitycorrectionfactor":	100
			}, {
				"gpionumber":	4,
				"gpiousage":	"flashlight-pwm",
				"pinenabled":	false,
				"pinname":	"",
				"pinmode":	"flashlight-default",
				"capturemode":	"cyclic-polling",
				"inputdebouncetime":	200,
				"pwmfrequency":	5000,
				"exposetomqtt":	true,
				"exposetorest":	true,
				"smartled":	{
					"type":	0,
					"quantity":	1,
					"colorredchannel":	255,
					"colorgreenchannel":	255,
					"colorbluechannel":	255
				},
				"intensitycorrectionfactor":	100
			}, {
				"gpionumber":	12,
				"gpiousage":	"spare",
				"pinenabled":	false,
				"pinname":	"",
				"pinmode":	"flashlight-smartled",
				"capturemode":	"cyclic-polling",
				"inputdebouncetime":	200,
				"pwmfrequency":	5000,
				"exposetomqtt":	true,
				"exposetorest":	true,
				"smartled":	{
					"type":	0,
					"quantity":	1,
					"colorredchannel":	255,
					"colorgreenchannel":	255,
					"colorbluechannel":	255
				},
				"intensitycorrectionfactor":	100
			}, {
				"gpionumber":	13,
				"gpiousage":	"spare",
				"pinenabled":	true,
				"pinname":	"",
				"pinmode":	"flashlight-digital",
				"capturemode":	"cyclic-polling",
				"inputdebouncetime":	200,
				"pwmfrequency":	5000,
				"exposetomqtt":	true,
				"exposetorest":	true,
				"smartled":	{
					"type":	0,
					"quantity":	1,
					"colorredchannel":	255,
					"colorgreenchannel":	255,
					"colorbluechannel":	255
				},
				"intensitycorrectionfactor":	100
			}]
	},
	"log":	{
		"debug":	{
			"loglevel":	2,
			"logfilesretention":	5,
			"debugfilesretention":	5
		},
		"data":	{
			"enabled":	true,
			"datafilesretention":	30
		}
	},
	"network":	{
		"wlan":	{
			"ssid":	"",
			"password":	"",
			"hostname":	"watermeter",
			"ipv4":	{
				"networkconfig":	0,
				"ipaddress":	"",
				"subnetmask":	"",
				"gatewayaddress":	"",
				"dnsserver":	""
			},
			"wlanroaming":	{
				"enabled":	false,
				"rssithreshold":	-75
			}
		},
		"time":	{
			"timezone":	"CET-1CEST,M3.5.0,M10.5.0/3",
			"ntp":	{
				"timesyncenabled":	true,
				"timeserver":	"",
				"processstartinterlock":	true
			}
		}
	},
	"system":	{
		"cpufrequency":	160
	},
	"webui":	{
		"autorefresh":	{
			"overviewpage":	{
				"enabled":	true,
				"refreshtime":	5
			},
			"datagraphpage":	{
				"enabled":	false,
				"refreshtime":	60
			}
		}
	}
}
```