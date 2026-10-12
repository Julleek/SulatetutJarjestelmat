*** Settings ***
Library           String
Library           SerialLibrary
Suite Setup       Avaa Portti
Suite Teardown    Sulje Portti
Test Setup        Reset Input Buffer
Test Template     Aikamerkkijono Palauttaa

*** Variables ***
${PORT}           COM3
${BAUD}           115200
${TIMEOUT}        3

*** Test Cases ***                      LAHETETAAN    ODOTETTU
# Oikeat merkkijonot
Pelkat sekunnit                         000059        59
Yksi minuutti                           000100        60
Minuutti ja 20 sekuntia                 000120        80
Maksimi minuutit ja sekunnit            005959        3599

# Virheelliset merkkijonot
Liian lyhyt                             00120         -1
Liian pitka                             0001200       -1
Kirjaimia numeroiden seassa             00a120        -3
Tunnit yli 23                           240000        -4
Minuutit yli 59                         006000        -5
Sekunnit yli 59                         001067        -6
Nolla aika                              000000        -7
Sekvenssivirhe (vaara kirjain)          RYGZ          -11

*** Keywords ***
Avaa Portti
    Add Port    ${PORT}    baudrate=${BAUD}    bytesize=8    parity=N    stopbits=1    timeout=${TIMEOUT}
    Open Port
    Sleep    2s
    Reset Input Buffer

Sulje Portti
    Close Port
    Delete All Ports

Aikamerkkijono Palauttaa
    [Arguments]    ${lahetetaan}    ${odotettu}

    Write Data    ${lahetetaan}\n    encoding=ascii
    ${read}=    Read Until    encoding=ascii
    Log To Console    ${lahetetaan} -> ${read}

    ${luvut}=    Get Regexp Matches    ${read}    -?\\d+
    Should Not Be Empty    ${luvut}    msg=Vastauksessa ei lukua: ${read}
    Should Be Equal As Integers    ${luvut}[0]    ${odotettu}
    ...    msg=Syote ${lahetetaan}: odotettiin ${odotettu}, vastaus oli: ${read}