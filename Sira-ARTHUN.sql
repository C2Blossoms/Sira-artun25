SELECT * FROM new_schema.students
INNER JOIN rfid ON students.id = rfid.idRFID;