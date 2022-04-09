' VBS Library.

' Raise critial error. It inttrupts server!
sub RaiseFatalError(msg)
    msgbox msg, vbOKOnly, "Critial Error - MinServer VBS"
end sub

dim send_target

' Get attributes first.
' ** After change script engine it's necessary to change these.
set args = WScript.Arguments
if args.count < 1 then
    RaiseFatalError "Bad argument passed by"
end if
send_target = args(0)
' ** End

set fso = CreateObject("Scripting.FileSystemObject")
set send_buf = fso.CreateTextFile(send_target)

' This function already add CRLF in the end.
sub Send(data)
    send_buf.WriteLine(data)
end sub

sub EndSend()
    send_buf.Close
end sub

'''''''''''''''''''''''''''''''''''''''''''''''


