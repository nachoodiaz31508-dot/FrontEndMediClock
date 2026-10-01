using Microsoft.AspNetCore.Components.Web;
using Microsoft.AspNetCore.Components.WebAssembly.Hosting;
using FrontEndMediClock;
using FrontEndMediClock.Servicios;

var builder = WebAssemblyHostBuilder.CreateDefault(args);
builder.RootComponents.Add<App>("#app");
builder.RootComponents.Add<HeadOutlet>("head::after");

// URL base configurable: wwwroot/appsettings.json (MediClock:BaseUrl).
// Default al backend productivo si no hay configuracion.
var baseUrl = builder.Configuration["MediClock:BaseUrl"] ?? "https://mediclockbackend.runasp.net";
builder.Services.AddScoped(sp => new HttpClient { BaseAddress = new Uri(baseUrl.TrimEnd('/') + "/") });
builder.Services.AddScoped<ApiMediClock>();
builder.Services.AddScoped<EstadoDispositivo>();
builder.Services.AddScoped<PreferenciasLocales>();

await builder.Build().RunAsync();
