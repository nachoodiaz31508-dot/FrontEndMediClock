using System.ComponentModel.DataAnnotations;

namespace FrontEndMediClock.Modelos;

// Dispositivo tal como lo envia el backend (wire camelCase).
// Se usan los mismos nombres que el JSON para no necesitar JsonPropertyName.
public class Dispositivo
{
    public int dispositivoId { get; set; }

    [Required(ErrorMessage = "El nombre es obligatorio.")]
    public string nombre { get; set; } = string.Empty;
}
