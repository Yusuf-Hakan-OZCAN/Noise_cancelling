# Noise_cancelling

GEREKLİ KÜTÜPHANE

1- Öncelikle main.c'de kullanılan FFmpeg kütüphanesini indirip kurmalısınız. 


KULLANIM

1- Ses dosyanız .wav formatında olmalıdır. Eğer başka bir format kullanmak istiyorsanız belirli değişiklikler yapmanız gerekir.


2- Ses dosyasını projenizin .exe dosyasının bulunduğu klasöre atın.("C:\Users\...\bin\Debug" benzeri bir yol) 

3- main.c klasörünüzün içinde bulunan .exe dosyanızın bulunduğu yolu kopyalayın.

4- Sonrasında cmd(komut istemi)'yi açarak "cd" komutu ile kopyaladığınız dizine gidin.

5- Debug klasörünüzün içinde bulunan .exe dosyasını ve bir boşluk bırakarak ses dosyanızı(örneğin ses.wav) yazarak enter tuşuna basın.

6- Sonrasında bilgisayarınızın hızına göre 5-10 saniye boyunca bekledikten sonra "Ses dosyaniz basariyla temizlenip kaydedildi: temiz_ses.wav" mesajını göreceksiniz.

7- Bu mesajı gördüğünüzde projenizin .exe dosyasının olduğu klasörde yeni bir ses dosyası oluştuğunu göreceksiniz. Bu arkadaki gürültüsü temizlenmiş yeni ses dosyasıdır.

8- Örneğin ses dosyanızın ismi gürültülüses.wav ise yeni oluşan gürültüsü temizlenmiş ses dosyası temiz_gürültülüses.wav ismiyle oluşturulacaktır.

9- Kodda yer alan DEFINE parametreleriyle oynayarak sesin belirginliğini, kalitesini veya arkadaki gürültünün azlık çokluğunu ayarlayabilirsiniz.
______________________________________________________________________________________________________________________________________________________________________________
REQUIRED LIBRARY

1- First of all, you should download and install the FFmpeg library used in main.c.


USE

1- Your audio file must be in .wav format. If you want to use another format you need to make changes in code.

2- Put the audio file in the folder where your project's .exe file is located (something like “C:\Users...\bin\Debug”)

3- Copy the path to your .exe file in your main.c folder.

4- Then open cmd (command prompt) and go to the directory you copied with the “cd” command.

5- Type the .exe file in your Debug folder and your audio file (for example, ses.wav) and press enter.

6- After waiting for 5-10 seconds depending on the speed of your computer, you will see the message "Ses dosyaniz basariyla temizlenip kaydedildi: temiz_ses.wav".

7- When you see this message, you will see that a new sound file has been created in the folder where your project's .exe file is located. This is the new sound file with the background noise removed.

8- For example, if the name of your sound file is noisy.wav, the newly created noise-cleaned sound file will be created with the name temiz_noisy.wav.

9- By changing with the DEFINE parameters in the code, you can adjust the clarity and quality of the sound or the amount of background noise.
