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
set req_buf = fso.CreateTextFile(req_target)

' Provides random integer in range
' Thanks to https://www.cnblogs.com/wangcp-2014/p/4223685.html
function GetRandomMath(m,n)
  Randomize
  GetRandomMath = Int(((n-m+1) * Rnd) + m)
end function

max_token = 65535

' This function already add CRLF in the end.
sub Send(data)
    send_buf.WriteLine(data)
end sub

function RequireUserToken(uid) 
    tk = GetRandomMath(1,max_token)
    while utokens.Exists(tk)
        tk = GetRandomMath(1,max_token)
    wend
    utokens.Add tk,uid
    req_buf.WriteLine("user_alloc " & uid & " " & tk)
    RequireUserToken = tk
end function

function RequireFileToken(fname,operate)
    tk = GetRandomMath(1,max_token)
    while ftokens.Exists(tk)
        tk = GetRandomMath(1,max_token)
    wend
    ftokens.Add tk,fname
    req_buf.WriteLine("file_alloc " & tk & " " & fname & " " & operate)
    RequireFileToken = tk
end function

' Automaticly call in the end of program.
' Don't call it manually.
sub EndOfProgram()
    send_buf.Close
    req_buf.Close
end sub

'''''''''''''''''''''''''''''''''''''''''''''''


