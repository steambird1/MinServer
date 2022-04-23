' VBS Library.

sub RaiseFatalError(msg)
    err.Raise 1, "MinServer VBS", msg
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

' Call in hook, to show this document is hooked
sub SetHook()
    req_buf.WriteLine("hook")
end sub

sub ReloadFile(filename)
    req_buf.WriteLine("reload " & filename)
end sub

sub ReloadAll()
    ReloadFile "#"
end sub

sub Sync(syntype, filename)
    req_buf.WriteLine("sync " & syntype & " " & filename)
end sub

sub SyncDirectory(directory)
    Sync "directory", directory
end sub

sub SyncFile(file)
    Sync "file", file
end sub

sub SyncAll()
    Sync "all", ""
end sub

' Automaticly send header.
sub SendHeader(ver, codeid, info, attrdict, content, autolen)
    s_ver = ver
    Send(s_ver & " " & codeid & " " & info)
    if not IsNull(attrdict) then
        for each i in attrdict
            Send(i & ": " & attrdict.Item(i))
        next
    end if
    if not IsNull(content) then
        if autolen then
            Send("Content-Length: " & len(content))
        end if
        Send("")    ' 2 LFs to symbol the beginning of content
        Send(content)
    end if
end sub

' Automaticly call in the end of program.
' Don't call it manually.
sub EndOfProgram()
    send_buf.Close
    req_buf.Close

    if err.number <> 0 then
        set err_buf = fso.CreateTextFile(err_target)
        err_buf.WriteLine("<p>Error ID: " & err.number & "</p>")
        err_buf.WriteLine("<p>Error Description: " & err.Description & "</p>")
        err_buf.WriteLine("<p>Error Source: " & err.Source & "</p>")
        err_buf.Close
        err.Clear()
    end if
end sub

'''''''''''''''''''''''''''''''''''''''''''''''


