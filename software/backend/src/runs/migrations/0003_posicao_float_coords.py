from django.db import migrations, models


class Migration(migrations.Migration):
    dependencies = [
        ("runs", "0002_posicao_bateria_posicao_orientacao_and_more"),
    ]

    operations = [
        migrations.AlterField(
            model_name="posicao",
            name="coordenada_x",
            field=models.FloatField(),
        ),
        migrations.AlterField(
            model_name="posicao",
            name="coordenada_y",
            field=models.FloatField(),
        ),
    ]
