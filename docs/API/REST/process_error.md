[Overview](_OVERVIEW.md) 

### REST API endpoint: process_error

`http://IP-ADDRESS/process_error`


Get actual process error state

Possible states:
- `000: No process error`
- `E90: Process error occured`
- `E91: Multiple process errors in row` (3 errors)


Payload:
- No payload needed

Response:
- Content type: `HTML`
- Content: Query response
- Example: `000: No process error`
