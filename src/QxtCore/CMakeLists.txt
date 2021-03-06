################################################################
# QxtCore
################################################################

#
# Name of project
#
PROJECT (QxtCore)

#
# QT include files
#
if(Qt5_FOUND)
    include_directories(${Qt5Core_INCLUDE_DIRS})
endif()
IF (QT4_FOUND)
    INCLUDE(${QT_USE_FILE})
ENDIF ()

#
# With AUTOMOC, do not need to specify files
# that contain Q_OBJECT macro for Qt to process with 'moc'
# (meta-object compiler).
#
IF(WORKBENCH_USE_CMAKE_AUTOMOC)
    SET(CMAKE_AUTOMOC ON)
ELSE()
    SET(MOC_INPUT_HEADER_FILES
        qxtcsvmodel.h
    )

    IF(Qt5_FOUND)
        QT5_WRAP_CPP(MOC_SOURCE_FILES ${MOC_INPUT_HEADER_FILES})
    ENDIF()
    IF (QT4_FOUND)
        QT4_WRAP_CPP(MOC_SOURCE_FILES ${MOC_INPUT_HEADER_FILES})
    ENDIF ()
ENDIF()


#
# Header files
#
SET(SOURCE_FILES
    qxtcsvmodel.h
    qxtglobal.h

    qxtcsvmodel.cpp
)

#
# Process the header files with moc producing moc_*.cpp files
#
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

#
# Create the GUI library
#
ADD_LIBRARY(QxtCore
    ${SOURCE_FILES}
    ${MOC_SOURCE_FILES}
)

TARGET_LINK_LIBRARIES(QxtCore ${CARET_QT5_LINK})

SET(QxtCore_LIBRARIES QxtCore PARENT_SCOPE)
SET(QxtCore_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
