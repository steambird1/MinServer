
' MS VBS Parameters
on error resume next
' Quote support
q = """"

http_proto = "%s"
method = "%s"
path = %s

' Content SAVE AS FILES, here shows path
content_path = "%s"

' HTTP Attributes.
set attr = CreateObject("Scripting.Dictionary")

%s

client_ip = "%s"

' User tokens and file tokens.
set utokens = CreateObject("Scripting.Dictionary")
set ftokens = CreateObject("Scripting.Dictionary")

%s

' Server settings.
set srvsetting = CreateObject("Scripting.Dictionary")

%s

' Set where could requests save to.
req_target = "%s"
err_target = "%s"

' Set the startup directory of the program.
start_directory = %s
kb_buf_usage = %ld

'''''''''''''''''''''

