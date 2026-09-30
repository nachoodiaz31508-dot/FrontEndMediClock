using System.ComponentModel.DataAnnotations;

namespace FrontEndMediClock.Modelos;

// Alarma tal como la envia el backend (wire camelCase).
// La hora viaja como texto "HH:mm:ss" porque el backend la expone asi;
// no se usa TimeSpan para no romper el contrato.
public class Alarma
{
    public int alarmaId { get; set; }

    // Dia de semana 1-7 (1 = domingo, segun contrato del backend).
    [Range(1, 7, ErrorMessage = "El dia de semana debe estar entre 1 y 7.")]
    public int diaSemana { get; set; }

    // Numero de alarma dentro del dia: 1-3.
    [Range(1, 3, ErrorMessage = "El numero de alarma debe estar entre 1 y 3.")]
    public int numeroAlarma { get; set; }

    // Formato contractual "HH:mm:ss" (ej: "08:00:00").
    [Required(ErrorMessage = "La hora es obligatoria.")]
    [RegularExpression(@"^([01]\d|2[0-3]):[0-5]\d:[0-5]\d$", ErrorMessage = "La hora debe tener formato HH:mm:ss.")]
    public string hora { get; set; } = string.Empty;

    public int dispositivoId { get; set; }
}
