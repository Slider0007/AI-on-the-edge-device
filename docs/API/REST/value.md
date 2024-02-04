[Overview](_OVERVIEW.md) 

### REST API endpoint: value

`http://IP-ADDRESS/value`


Show process result


Payload:
1. Return data from all number sequences:
    - Actual Value: `/value?all=true&type=value`
    - Fallback Value: `/value?all=true&type=fallback`
    - Raw Value: `/value?all=true&type=raw`
    - Value Status: `/value?all=true&type=status`

2. Return data from a specific number sequence with e.g. name `main`:
    - Actual Value: `/value?all=true&type=value&numbersname=main`
    - Raw Value: `/value?all=true&type=raw&numbersname=main`
    - Fallback Value: `/value?all=true&type=fallback&numbersname=main`
    - Value Status: `/value?all=true&type=status&numbersname=main`

3. Retrieve WebUI recognition page content, use `/value?full=true`


Response:
- Content type: `HTML`
- Content: Query response (sequence name + result; Tab devided)