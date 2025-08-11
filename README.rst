Configuração do Ambiente Asa Virtual
====================================

1. Instalação do FlightGear 2020.3.19:

    1. Faça o download o FlightGear 2020.3.19 (https://sourceforge.net/projects/flightgear/files/release-2020.3/FlightGear-2020.3.19.exe/download)
    2. Siga as instruções e mantenha as configurações padrão durante a instação;
    3. Configuração do FlightGear para envio dos dados para o FGMS:
    
       1. Abra o FlightGear
       2. Selecione o menu lateral **Settings**;
       3. No campo **Additional Settings** entre com o valor (ver imagem abaixo):
         `--multiplay=out,10,127.0.0.1,5000 --multiplay=in,10,127.0.0.1,5001 --callsign=FG01 --httpd=8001` 

        .. image:: docs/imgs/settings.png
            :width: 500
            :alt: Seleção de Aircraft


        .. note::
            Se for estabelecer conexão com outros FlightGear em rede, utilize nomes únicos entre cada player da rede.




2. Instalação da Aeronave F-16CJ Block 52:

    O modelo de F-16CJ usado pode ser encontrado nesse `repositório <https://github.com/NikolaiVChr/f16/tree/master>`__, onde é possível encontrar `instruções de instalação <https://github.com/NikolaiVChr/f16/wiki/Installation-instructions>`__ atualizadas.

    Para instalar esse modelo no FlightGear, siga o seguinte roteiro:
    
    1. Fecho o FlightGear, caso esteja aberto;
    2. Baixe o F-16CJ de `https://github.com/NikolaiVChr/f16/archive/refs/heads/master.zip <https://github.com/NikolaiVChr/f16/archive/refs/heads/master.zip>`__;
    3. Descompacte o arquivo master.zip;
    4. Dentro da pasta descompactada você encontrará uma pasta chamada **f16-master**. Renomeie a pasta **f16-master** para **f16**.;
    5. Copie a pasta **f16** para `C:\Program Files\FlightGear 2020.3\data\Aircraft\`.
    
    
    6. Configuração do F-16CJ no FlightGear:
      1. Abra o FlightGear;
      2. Selecione o menu lateral **Aircraft** e selecione na lista superior **Installed Aircraft**;
      3. Verifique se a opção  **General Dynamics F-16CJ Block 52** está selecionada;


        .. image:: docs/imgs/aircraft.png
            :width: 500
            :alt: Seleção de Aircraft


3. Instalação do FGMS (FlightGear Multiplayer Server)

    1. Utilizando um terminal WSL, baixe o repositório se não ainda não possuir em sua máquina:
       .. code-block:: bash

            git clone https://github.com/ASA-Simulation/AsaFG.git 


    2. Entre na pasta do fmgs do repositório AsaFG, atualize o repositório e rode o FGMS:

        .. code-block:: bash

            cd AsaFG
            git checkout development
            git pull
            make fgms-start

        .. note::
            Para fechar o servidor FGMS, digite Ctrl+C no terminal do servidor.

    3. Caso queira monitorar a quantidade de players conectados no servidor FGMS, abra um novo terminal WSL, entre na pasta do projeto asa-fg e execute:

        .. code-block:: bash

            make fgms-watch


4. Instalação da biblioteca SimGear:

    A biblioteca SimGear é/será utilizado pelo XDR Client para realizar comunicação entre o ASA e o FGMS.

    1. Utilizando um terminal WSL, digite o seguinte comando para instalar a biblioteca SimGear

        .. code-block:: bash

            apt-get install libsimgear-dev


5. Instalação e uso do client XDR (exemplo de comunicação com o FGMS)

    1. Utilizando um terminal WSL, entre na pasta  `AsaFG/tests/xdr_client`. Utilize o seguinte comando para compilar o código:

        .. code-block:: bash

            make build

    2. Rode a aplicação:

        .. code-block:: bash

            ./xdr_client



.. note
    Para efeitos de crédito, esse tutorial usou como referência as documentações elaboradas pelos ASP OF ENG ARTHUR José de Souza Rodrigues e ASP OF ENG THIAGO LOPES de Araujo 