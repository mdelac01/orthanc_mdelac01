var pendingUploads = [];
var currentUpload = 0;
var totalUpload = 0;
var alreadyInitialized = false; // trying to debug Orthanc issue #1

$(document).ready(function() {
  if (alreadyInitialized) {
    console.log("Orthanc issue #1: the fileupload has been initialized twice !");
  } else {
    alreadyInitialized = true;
  }
  
  // Initialize the jQuery File Upload widget:
  $('#fileupload').fileupload({
    //dataType: 'json',
    //maxChunkSize: 500,
    //sequentialUploads: true,
    limitConcurrentUploads: 3,
    add: function (e, data) {
      pendingUploads.push(data);
    }
  })
    .bind('fileuploadstop', function(e, data) {
      $('#upload-button').removeClass('ui-disabled');
      //$('#upload-abort').addClass('ui-disabled');
      $('#progress .bar').css('width', '100%');
      if ($('#progress .label').text() != 'Failure')
        $('#progress .label').text('Done');
    })
    .bind('fileuploadfail', function(e, data) {
      $('#progress .bar')
        .css('width', '100%')
        .css('background-color', 'red');
      $('#progress .label').text('Failure');
    })
    .bind('fileuploaddrop', function (e, data) {
      console.log("dropped " + data.files.length + " files: ", data);
      appendFilesToUploadList(data.files);
    })
    .bind('fileuploadsend', function (e, data) {
      // Update the progress bar. Note: for some weird reason, the
      // "fileuploadprogressall" does not work under Firefox.
      var progress = parseInt(currentUpload / totalUploads * 100, 10);
      currentUpload += 1;
      $('#progress .label').text('Uploading: ' + progress + '%');
      $('#progress .bar')
        .css('width', progress + '%')
        .css('background-color', 'green');
    });
});

function appendFilesToUploadList(files) {
  var target = $('#upload-list');
  $.each(files, function (index, file) {
    target.append('<li class="pending-file">' + file.name + '</li>');
  });
  target.listview('refresh');
}

$('#fileupload').live('change', function (e) {
  appendFilesToUploadList(e.target.files);
})


function ClearUploadProgress()
{
  $('#progress .label').text('');
  $('#progress .bar').css('width', '0%').css('background-color', '#333');
}

$('#upload').live('pagebeforeshow', function() {
  if (navigator.userAgent.toLowerCase().indexOf('firefox') == -1) {
    $("#issue-21-warning").css('display', 'none');
  }

  ClearUploadProgress();
});

$('#upload').live('pageshow', function() {
  $('#fileupload').fileupload('enable');
});

$('#upload').live('pagehide', function() {
  $('#fileupload').fileupload('disable');
});


$('#upload-button').live('click', function() {
  var pu = pendingUploads;
  pendingUploads = [];

  $('.pending-file').remove();
  $('#upload-list').listview('refresh');
  ClearUploadProgress();

  currentUpload = 1;
  totalUploads = pu.length + 1;
  if (pu.length > 0) {
    $('#upload-button').addClass('ui-disabled');
    //$('#upload-abort').removeClass('ui-disabled');
  }

  for (var i = 0; i < pu.length; i++) {
    pu[i].submit();
  }
});

$('#upload-clear').live('click', function() {
  pendingUploads = [];
  $('.pending-file').remove();
  $('#upload-list').listview('refresh');
});

/*$('#upload-abort').live('click', function() {
  $('#fileupload').fileupload().abort();
  });*/
