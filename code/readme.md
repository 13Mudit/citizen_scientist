### 2 types of data required from C back end:
	- Stream ADC data
		- Will be slower since regular communication required
		- Will be batched/in discrete packets, packet size dependent on ARR_LEN
	- Single shot data:
		- Size dependent on ARR_LEN

### TCP message format
'0 *ARR_LEN*' -> Stream data
	for continuous conversation, send '1'
	if want to close send '0'

'1 *ARR_LEN*' -> Single shot data