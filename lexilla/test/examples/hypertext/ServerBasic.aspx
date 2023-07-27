<%@ register tagprefix="uc1" 
    tagname="CalendarUserControl" 
    src="~/CalendarUserControl.ascx" %>
<!DOCTYPE html>
<html>
<%@language=VBScript%>
<%-- comment --%>
<script type="text/vbscript">
'1%>2
'1?>2
'%>
'?>
</script>
<script type="text/vbscript">
dim e="%>"
dim f="?>"
</script>
Start
<%response.write("1")%>
<% 'comment%>
<%dim x="2"'comment%>
<%response.write(x)%>
End
</html>
