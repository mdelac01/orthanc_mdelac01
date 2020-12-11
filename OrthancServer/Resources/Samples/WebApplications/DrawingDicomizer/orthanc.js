/**
 * Orthanc - A Lightweight, RESTful DICOM Store
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


function guid4Block() {
  return Math.floor((1 + Math.random()) * 0x10000)
    .toString(16)
    .substring(1);
}
 
function guid() {
  return (guid4Block() + guid4Block() + '-' + guid4Block() + '-' + guid4Block() + '-' +
          guid4Block() + '-' + guid4Block() + guid4Block() + guid4Block());
}


$(document).ready(function() {
  $('#patientID').val(guid());

  $('#submit').click(function(event) {
    var png = context.canvas.toDataURL();

    $.ajax({
      type: 'POST',
      url: '/orthanc/tools/create-dicom',
      dataType: 'text',
      data: { 
        PatientID: $('#patientID').val(),
        PatientName: $('#patientName').val(),
        StudyDescription: $('#studyDescription').val(),
        SeriesDescription: $('#seriesDescription').val(),
        PixelData: png,
        Modality: 'RX'
      },
      success : function(msg) {
        alert('Your drawing has been DICOM-ized!\n\n' + msg);
      },
      error : function() {
        alert('Error while DICOM-izing the drawing');
      }
    });

    return false;
  });
});
