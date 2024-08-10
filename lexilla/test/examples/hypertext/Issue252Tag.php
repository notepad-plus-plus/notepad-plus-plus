<!-- Check PHP start tags for issue 252 -->

<!-- Full PHP tag: enabled -->
<?php
echo __FILE__.__LINE__;
?>

<!-- Short echo tag: enabled -->
<?= 'echo' ?>

<!-- Short tag: disabled -->
<?
echo 'short'
?>

<!-- ASP language selection: disabled -->
<%@language=JScript%>

<!-- ASP tag: disabled -->
<%
Response.Write('short')
%>
