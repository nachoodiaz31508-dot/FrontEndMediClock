using System.ComponentModel.DataAnnotations;

namespace FrontEndMediClock.Modelos;

// Evento tal como lo envia el backend (wire camelCase).
// fechaHora viaja como texto ISO porque el servidor es quien la asigna;
// el frontend nunca la envia (el server la pone al crear el evento).
public class Evento
{
    public int eventoId { get; set; }

    public string fechaHora { get; set; } = string.Empty;

    // Union esperada: valores como "AlarmaActivada", "DosisEntregada", etc.
    // Se deja como string para no romper si el backend agrega tipos nuevos.
    public string tipo { get; set; } = string.Empty;

    [StringLength(300, MinimumLength = 10, ErrorMessage = "La descripcion debe tener entre 10 y 300 caracteres.")]
    public string descripcion { get; set; } = string.Empty;

    public int dispositivoId { get; set; }

    // Nulo cuando el evento no esta asociado a una alarma.
    public int? alarmaId { get; set; }
}
