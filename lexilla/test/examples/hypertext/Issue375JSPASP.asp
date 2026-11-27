<!-- Issue 375. Add more server side starts for JSP and ASP.
For <%! %>, <%# %>, <%$ %>, and <%: %>, the character following
the server side sequence should be included in SCE_H_ASP.
The above examples are JSP declaration, ASP data-binding, ASP expression builder, ASP encoded expression.
-->

<%@language=javascript%>
<%! int i = 0; %>
About to increment i...
<% i++; %>
<%= (new java.util.Date()).toLocaleString() %>
 
<%@language=vbscript%>
<%=DateTime.Now.ToString() %>
<asp:Label ID="lblHello" runat="server" Text="<%# SayHello%>"></asp:Label>
<asp:Literal ID="Literal1" runat="server" Text="<%$ AppSettings: copyright %>"></asp:Literal>
<%:DateTime.Now.ToString() %>
