[Overview](_OVERVIEW.md) 

## REST API endpoint: config

`http://IP-ADDRESS/config`

Get and set process-related configuration parameters. The data is provided / needs to be provided in JSON notation.<br>
Parameter descriptions for every single parameter are located at the GitHub repository (`docs/Configuration/Parameter`) 
or can be displayed on the WebUI configuration page (question mark symbol next to each parameter).

- JSON: `/config`
- HTML: `/config?task=reload`

1. **Get API name and version:**
    - Payload:
      - `/config?task=api_name`
    - Response:
      - Content type: `HTML`
      - Content: HTML query response, e.g. `config:v1`

2. **HTML query request to reload configuration and reinitialize process:**
    - Payload:
      - `/config?task=reload`
    - Response:
      - Content type: `HTML`
      - Content: HTML query response

3. **Get config in JSON notation (GET handler)**
    - Payload:
      - No payload needed
    - Response:
      - Content type: `JSON`
      - Content: JSON response
    - Example: see below

4. **Set config in JSON notation (POST handler)**
    - Payload:
      - Configuration in JSON notation
      - Setting only a single, some, or all parameters is supported
      - Optional query parameter `reload=true` can be used to reload the configuration and reinitialize the 
      process after the configuration has been successfully saved
      - `reload=1` can be used as an alternative to `reload=true`
    - Response:
      - POST handler status response
      - When `reload=true` or `reload=1` is used, reload status is returned in the `X-Reload-Code` and 
      `X-Reload-Message` response headers
    - Examples:
      - `/config`
      - `/config?reload=true`
      - `/config?reload=1`

    Example: see below


```
{
	"config":	{
		"version":	x,
		"lastmodified":	"2024-09-18T00:09:06+0200"
	},
	"operationmode":	{
		"opmode":	1,
		"automaticprocessinterval":	"1.0",
		"usedemoimages":	false
	},
	"takeimage":	{
		"flashlight":	{
			"flashtime":	2000,
			"flashintensity":	20
		},
		"camera":	{
			...
		}
	},
	...
}
```