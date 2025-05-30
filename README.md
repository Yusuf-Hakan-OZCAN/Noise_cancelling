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
