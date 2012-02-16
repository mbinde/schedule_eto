(function($){
	$(function() {
		$("#choose-date").datepicker({
   			onSelect: function(dateText, inst) {
   				$("#date-details").html('Loading schedule for <strong>' + dateText + '</strong>...');
   				path = window.location.pathname.split("/");
   				$("#date-details").load("/callback/schedule", {date:dateText,user:path[2],path1:path[1]});
   			}
		});
	});
	$(function() {
		$("#show-calendar").datepicker({
   			onSelect: function(dateText, inst) {
   				$("#date-details").html('Loading schedule for <strong>' + dateText + '</strong>...');
   				path = window.location.pathname.split("/");
   				$("#date-details").load("/callback/calendar", {date:dateText,user:path[2],path1:path[1]});
   			}
		});
	});
})(jQuery);